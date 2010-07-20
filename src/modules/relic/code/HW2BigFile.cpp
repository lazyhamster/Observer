#include "stdafx.h"
#include "HW2BigFile.h"

#define RETNOK(x) if (!x) return false

CHW2BigFile::CHW2BigFile()
{

}

CHW2BigFile::~CHW2BigFile()
{
	Close();
}

bool CHW2BigFile::Open( HANDLE inFile )
{
	Close();

	if (inFile == INVALID_HANDLE_VALUE)
		return false;

	LARGE_INTEGER nInputFileSize;
	RETNOK( GetFileSizeEx(inFile, &nInputFileSize) );

	m_hInputFile = inFile;
	RETNOK( SeekPos(0, FILE_BEGIN) );

	// Read and check archive header params
	RETNOK( ReadData(&m_archHeader, sizeof(m_archHeader)) );
	RETNOK( strncmp(m_archHeader.Magic, "_ARCHIVE", 8) == 0 );
	RETNOK( (m_archHeader.exactFileDataOffset < nInputFileSize.QuadPart) );

	// Read section header
	RETNOK( ReadData(&m_sectionHeader, sizeof(m_sectionHeader)) );

	BIG_TOCListEntry* vTOC = NULL;
	BIG_FolderListEntry* vFolderList = NULL;
	BIG_FileInfoListEntry* vFileInfoList = NULL;

	RETNOK( ReadStructArray<BIG_TOCListEntry>(m_sectionHeader.TOCList, &vTOC) );
	RETNOK( ReadStructArray<BIG_FolderListEntry>(m_sectionHeader.FolderList, &vFolderList) );
	RETNOK( ReadStructArray<BIG_FileInfoListEntry>(m_sectionHeader.FileInfoList, &vFileInfoList) );

	bool fResult = true;

	RETNOK( SeekPos(m_sectionHeader.FileNameList.Offset + sizeof(BIG_ArchHeader), FILE_BEGIN) );
	size_t nNameArraySize = m_archHeader.sectionHeaderSize - m_sectionHeader.FileNameList.Offset;
	char* szNamesListCache = (char *) malloc(nNameArraySize);
	fResult = ReadData(szNamesListCache, nNameArraySize);

	// Compose items list
	if (fResult)
	{
		for (uint16_t i = 0; i < m_sectionHeader.TOCList.Count; i++)
		{
			BIG_TOCListEntry &tocEntry = vTOC[i];
			char* szPathPrefix = tocEntry.CharacterAliasName[0] != 0 ? tocEntry.CharacterAliasName : tocEntry.CharacterName;
			fResult = FetchFolder(vFolderList, tocEntry.StartFolderIndexForHierarchy, vFileInfoList, szPathPrefix, szNamesListCache);

			if (!fResult) break;
		} // for i
	}

	free(szNamesListCache);
	
	m_bIsValid = fResult;
	return fResult;
}

void CHW2BigFile::Close()
{
	if (m_hInputFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hInputFile);
		m_hInputFile = INVALID_HANDLE_VALUE;
	}

	m_vItems.clear();
	m_bIsValid = false;
}

bool CHW2BigFile::FetchFolder( BIG_FolderListEntry* folders, int folderIndex, BIG_FileInfoListEntry* files,
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
		RETNOK( FetchFolder(folders, i, files, prefix, namesBuf) );
	}

	// Get files from current folder
	char szFileNameBuf[MAX_PATH] = {0};
	for (uint16_t i = folderInfo.FirstFilenameIndex; i < folderInfo.LastFilenameIndex; i++)
	{
		BIG_FileInfoListEntry &fileInfo = files[i];
		sprintf_s(szFileNameBuf, MAX_PATH, "%s\\%s", szNameBuf, namesBuf + fileInfo.FileNameOffset);

		// Fill item info
		HWStorageItem new_item = {0};
		MultiByteToWideChar(CP_ACP, 0, szFileNameBuf, strlen(szFileNameBuf), new_item.Name, MAX_PATH);
		new_item.CompressedSize = fileInfo.CompressedLength;
		new_item.UncompressedSize = fileInfo.DecompressedLength;
		new_item.DataOffset = fileInfo.FileDataOffset;
		new_item.Flags = fileInfo.Flags;

		BIG_FileHeader fileHeader;
		RETNOK( SeekPos(m_archHeader.exactFileDataOffset + fileInfo.FileDataOffset - sizeof(fileHeader), FILE_BEGIN) );
		RETNOK( ReadData(&fileHeader, sizeof(fileHeader)) );

		new_item.CRC = fileHeader.UncompressedDataCRC;
		// TODO: convert timestamp to FILETIME

		m_vItems.push_back(new_item);
	}

	return true;
}
