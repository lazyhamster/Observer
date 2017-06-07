#include "stdafx.h"
#include "HW2BigFile.h"
#include "zlib.h"

#define RETNOK(x) if (!x) return false

template<typename T>
bool ReadStructArray( CBasicFile* inFile, BIG_SectionRef secref, T** list )
{
	if (secref.Count > 10000) return false;

	if (!inFile->Seek(secref.Offset + sizeof(BIG_ArchHeader), FILE_BEGIN))
		return false;

	size_t nMemSize = secref.Count * sizeof(T);
	T* data = (T*) malloc(nMemSize);
	if (data == NULL) return false;

	bool fOpRes = inFile->ReadExact(data, (DWORD) nMemSize);
	if (fOpRes)
		*list = data;
	else
		free(data);

	return fOpRes;
}

//////////////////////////////////////////////////////////////////////////

CHW2BigFile::CHW2BigFile()
{
	memset(&m_archHeader, 0, sizeof(m_archHeader));
	memset(&m_sectionHeader, 0, sizeof(m_sectionHeader));
	m_vFileInfoList = NULL;
}

CHW2BigFile::~CHW2BigFile()
{
	BaseClose();
}

bool CHW2BigFile::Open( CBasicFile* inFile )
{
	// If file is already open then exit
	if (!inFile || !inFile->IsOpen() || m_bIsValid)
		return false;

	RETNOK( inFile->Seek(0, FILE_BEGIN) );

	// Read and check archive header params
	RETNOK( inFile->ReadExact(&m_archHeader, sizeof(m_archHeader)) );
	RETNOK( strncmp(m_archHeader.Magic, "_ARCHIVE", 8) == 0 );
	RETNOK( ((__int64) m_archHeader.exactFileDataOffset < inFile->GetSize()) );

	// Read section header
	RETNOK( inFile->ReadExact(&m_sectionHeader, sizeof(m_sectionHeader)) );

	BIG_TOCListEntry* vTOC = NULL;
	BIG_FolderListEntry* vFolderList = NULL;

	RETNOK( ReadStructArray<BIG_TOCListEntry>(inFile, m_sectionHeader.TOCList, &vTOC) );
	RETNOK( ReadStructArray<BIG_FolderListEntry>(inFile, m_sectionHeader.FolderList, &vFolderList) );
	RETNOK( ReadStructArray<BIG_FileInfoListEntry>(inFile, m_sectionHeader.FileInfoList, &m_vFileInfoList) );

	bool fResult = true;

	RETNOK( inFile->Seek(m_sectionHeader.FileNameList.Offset + sizeof(BIG_ArchHeader), FILE_BEGIN) );
	size_t nNameArraySize = m_archHeader.sectionHeaderSize - m_sectionHeader.FileNameList.Offset;
	char* szNamesListCache = (char *) malloc(nNameArraySize);
	fResult = inFile->ReadExact(szNamesListCache, (DWORD) nNameArraySize);

	// Compose items list
	if (fResult)
	{
		for (uint16_t i = 0; i < m_sectionHeader.TOCList.Count; i++)
		{
			BIG_TOCListEntry &tocEntry = vTOC[i];
			char* szPathPrefix = (tocEntry.CharacterAliasName[0] && (m_sectionHeader.TOCList.Count == 1)) ? tocEntry.CharacterAliasName : tocEntry.CharacterName;
			fResult = FetchFolder(inFile, vFolderList, tocEntry.StartFolderIndexForHierarchy, m_vFileInfoList, szPathPrefix, szNamesListCache);

			if (!fResult) break;
		} // for i
	}

	free(szNamesListCache);
	free(vTOC);
	free(vFolderList);
	
	m_bIsValid = fResult;
	if (m_bIsValid) m_pInputFile = inFile;

	return fResult;
}

bool CHW2BigFile::FetchFolder( CBasicFile* inFile, BIG_FolderListEntry* folders, int folderIndex, BIG_FileInfoListEntry* files,
							  const char* prefix, const char* namesBuf )
{
	BIG_FolderListEntry &folderInfo = folders[folderIndex];
	
	char szNameBuf[MAX_PATH] = {0};
	strcpy_s(szNameBuf, MAX_PATH, prefix);
	if (*(namesBuf + folderInfo.FileNameOffset) != 0)
	{
		strcat_s(szNameBuf, MAX_PATH, "\\");
		strcat_s(szNameBuf, MAX_PATH, namesBuf + folderInfo.FileNameOffset);
	}

	// Traverse to sub-folders
	for (uint16_t i = folderInfo.FirstSubfolderIndex; i < folderInfo.LastSubfolderIndex; i++)
	{
		RETNOK( FetchFolder(inFile, folders, i, files, prefix, namesBuf) );
	}

	// Get files from current folder
	char szFileNameBuf[MAX_PATH] = {0};
	for (uint16_t i = folderInfo.FirstFilenameIndex; i < folderInfo.LastFilenameIndex; i++)
	{
		BIG_FileInfoListEntry &fileInfo = files[i];
		sprintf_s(szFileNameBuf, MAX_PATH, "%s\\%s", szNameBuf, namesBuf + fileInfo.FileNameOffset);

		// Fill item info
		HWStorageItem new_item = {0};
		MultiByteToWideChar(CP_ACP, 0, szFileNameBuf, -1, new_item.Name, MAX_PATH);
		new_item.CompressedSize = fileInfo.CompressedLength;
		new_item.UncompressedSize = fileInfo.DecompressedLength;
		new_item.DataOffset = fileInfo.FileDataOffset;
		new_item.Flags = fileInfo.Flags;
		new_item.CustomData = i;

		BIG_FileHeader fileHeader;
		RETNOK( inFile->Seek(m_archHeader.exactFileDataOffset + fileInfo.FileDataOffset - sizeof(fileHeader), FILE_BEGIN) );
		RETNOK( inFile->ReadExact(&fileHeader, sizeof(fileHeader)) );

		new_item.CRC = fileHeader.UncompressedDataCRC;
		new_item.ModTime = fileHeader.FileModificationDate;

		m_vItems.push_back(new_item);
	}

	return true;
}

bool CHW2BigFile::ExtractFile( int index, HANDLE outfile )
{
	if (m_vFileInfoList == NULL) return false;
	
	HWStorageItem item = m_vItems[index];
	BIG_FileInfoListEntry &fileInfo = m_vFileInfoList[item.CustomData];

	RETNOK( m_pInputFile->Seek(m_archHeader.exactFileDataOffset + fileInfo.FileDataOffset, FILE_BEGIN) );
	bool fShouldDecompress = (fileInfo.Flags != BIGARCH_FILE_FLAG_UNCOMPRESSED);

	if (fShouldDecompress)
		return ExtractCompressed(fileInfo, outfile, item.CRC);
	else
		return ExtractPlain(fileInfo, outfile, item.CRC);
}

bool CHW2BigFile::ExtractPlain( BIG_FileInfoListEntry &fileInfo, HANDLE outfile, uint32_t fileCRC )
{
	const uint32_t BUF_SIZE = 16 * 1024;
	unsigned char inbuf[BUF_SIZE] = {0};

	uLong crc = crc32(0, Z_NULL, 0);

	DWORD nWritten;
	uint32_t nBytesLeft = fileInfo.CompressedLength;
	while (nBytesLeft > 0)
	{
		uint32_t nReadSize = (nBytesLeft > BUF_SIZE) ? BUF_SIZE : nBytesLeft;
		RETNOK( m_pInputFile->ReadExact(inbuf, nReadSize) );

		if (!WriteFile(outfile, inbuf, nReadSize, &nWritten, NULL) || (nWritten != nReadSize))
			return false;

		crc = crc32(crc, inbuf, nReadSize);
		nBytesLeft -= nReadSize;
	} // while

	return (crc == fileCRC);
}

bool CHW2BigFile::ExtractCompressed( BIG_FileInfoListEntry &fileInfo, HANDLE outfile, uint32_t fileCRC )
{
	z_stream strm = {0};
	int ret = inflateInit(&strm);
	if (ret != Z_OK) return false;

	const uint32_t BUF_SIZE = 32 * 1024;
	unsigned char inbuf[BUF_SIZE] = {0};
	unsigned char outbuf[BUF_SIZE] = {0};

	uLong crc = crc32(0, Z_NULL, 0);

	DWORD nWritten;
	uint32_t nBytesLeft = fileInfo.CompressedLength;
	while (nBytesLeft > 0)
	{
		uint32_t nReadSize = (nBytesLeft > BUF_SIZE) ? BUF_SIZE : nBytesLeft;
		RETNOK( m_pInputFile->ReadExact(inbuf, nReadSize) );

		strm.next_in = inbuf;
		strm.avail_in = nReadSize;
		do 
		{
			strm.avail_out = BUF_SIZE;
			strm.next_out = outbuf;

			ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_NEED_DICT) ret = Z_DATA_ERROR;
			if (ret < 0) break;

			unsigned int have = BUF_SIZE - strm.avail_out;
			if (have == 0) break;

			if (!WriteFile(outfile, outbuf, have, &nWritten, NULL) || (nWritten != have))
			{
				ret = Z_STREAM_ERROR;
				break;
			}

			crc = crc32(crc, outbuf, have);
		} while (strm.avail_out == 0);
		
		if (ret < 0 && ret != Z_BUF_ERROR) break;
		nBytesLeft -= nReadSize;
	} // while
	
	inflateEnd(&strm);
	return (ret == Z_STREAM_END) && (crc == fileCRC);
}
