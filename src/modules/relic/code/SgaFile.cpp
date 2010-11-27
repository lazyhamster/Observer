#include "stdafx.h"
#include "SgaFile.h"
#include "SgaStructs.h"

#define RETNOK(x) if (!x) return false

static bool IsSupportedVersion(unsigned short major, unsigned short minor)
{
	return (minor == 0) && (major == 2 || major == 4 || major == 5);
}

static bool ProcessDirEntry(directory_raw_info_t *dirList, file_info_t *fileList, int dirIndex, const char* namesPool, unsigned long dataBaseOffset, std::vector<HWStorageItem> &destList)
{
	const directory_raw_info_t &sourceDir = dirList[dirIndex];
	const char* sourceDirName = namesPool + sourceDir.iNameOffset;

	for (unsigned short i = sourceDir.iFirstDirectory; i < sourceDir.iLastDirectory; i++)
	{
		if (!ProcessDirEntry(dirList, fileList, i, namesPool, dataBaseOffset, destList))
			return false;
	}

	char szFileNameBuf[MAX_PATH];
	for (unsigned short i = sourceDir.iFirstFile; i < sourceDir.iLastFile; i++)
	{
		const file_info_t &file = fileList[i];
		const char* fileName = namesPool + file.iNameOffset;

		if (strlen(sourceDirName) > 0)
			sprintf_s(szFileNameBuf, MAX_PATH, "%s\\%s", sourceDirName, fileName);
		else
			strcpy_s(szFileNameBuf, MAX_PATH, fileName);

		HWStorageItem new_item = {0};
		MultiByteToWideChar(CP_ACP, 0, szFileNameBuf, -1, new_item.Name, MAX_PATH);
		new_item.CompressedSize = file.iDataLengthCompressed;
		new_item.UncompressedSize = file.iDataLength;
		new_item.DataOffset = file.iDataOffset + dataBaseOffset;
		new_item.Flags = file.iFlags32;
		new_item.CustomData = i;

		//new_item.CRC = fileHeader.UncompressedDataCRC;
		UnixTimeToFileTime(file.iModificationTime, &new_item.ModTime);

		destList.push_back(new_item);
	}
	
	return true;
}

CSgaFile::CSgaFile()
{

}

CSgaFile::~CSgaFile()
{
	BaseClose();
}

bool CSgaFile::Open( CBasicFile* inFile )
{
	// If file is already open then exit
	if (!inFile || !inFile->IsOpen() || m_bIsValid)
		return false;

	RETNOK( inFile->Seek(0, FILE_BEGIN) );

	file_header_t m_oFileHeader = {0};
	data_header_t m_oDataHeader = {0};
	
	char *m_sStringBlob = NULL;
	long m_iDataHeaderOffset = 0;

	RETNOK( inFile->ReadExact(m_oFileHeader.sIdentifier, 8) );
	RETNOK( strncmp(m_oFileHeader.sIdentifier, "_ARCHIVE", 8) == 0 );
	
	RETNOK( inFile->ReadValue(m_oFileHeader.iVersionMajor) );
	RETNOK( inFile->ReadValue(m_oFileHeader.iVersionMinor) );
	RETNOK( IsSupportedVersion(m_oFileHeader.iVersionMajor, m_oFileHeader.iVersionMinor) );

	RETNOK( inFile->ReadArray(m_oFileHeader.iContentsMD5, 16) );
	RETNOK( inFile->ReadArray(m_oFileHeader.sArchiveName, 64) );
	RETNOK( inFile->ReadArray(m_oFileHeader.iHeaderMD5, 16) );
	RETNOK( inFile->ReadValue(m_oFileHeader.iDataHeaderSize) );
	RETNOK( inFile->ReadValue(m_oFileHeader.iDataOffset) );

	if(m_oFileHeader.iVersionMajor == 5)
	{
		RETNOK( inFile->ReadValue(m_iDataHeaderOffset) );
	}

	if(m_oFileHeader.iVersionMajor >= 4)
	{
		long nPlatform;
		RETNOK( inFile->ReadValue(nPlatform) );
		if (nPlatform != 1) return false;  // Only Win platform is accepted
	}

	if(m_oFileHeader.iVersionMajor == 5)
	{
		RETNOK( inFile->Seek(m_iDataHeaderOffset, FILE_BEGIN) );
	}
	else
	{
		m_iDataHeaderOffset = (long) inFile->GetPosition();
	}

	//TODO: finish
	/*{
		MD5Hash oHeaderHash;
		long iHeaderHash[4];
		oHeaderHash.updateFromString("DFC9AF62-FC1B-4180-BC27-11CCE87D3EFF");
		oHeaderHash.updateFromFile(pSgaFile, header.iDataHeaderSize);
		oHeaderHash.finalise((unsigned char*)iHeaderHash);
		if (memcmp(iHeaderHash, header.iHeaderMD5, 16) != 0)
			return false; // Stored header hash does not match computed header hash
	}*/

	RETNOK( inFile->Seek(m_iDataHeaderOffset, FILE_BEGIN) );
	RETNOK( inFile->ReadValue(m_oDataHeader) );

	// Pre-load file and directory names
	RETNOK( inFile->Seek(m_iDataHeaderOffset + m_oDataHeader.iStringOffset, FILE_BEGIN) );
	size_t iStringsLength = m_oFileHeader.iDataHeaderSize - m_oDataHeader.iStringOffset;
	m_sStringBlob = (char*) malloc(iStringsLength);
	RETNOK( inFile->ReadArray(m_sStringBlob, iStringsLength) );

	// Read directory entries
	directory_raw_info_t* dirList = new directory_raw_info_t[m_oDataHeader.iDirectoryCount];
	RETNOK( inFile->Seek(m_iDataHeaderOffset + m_oDataHeader.iDirectoryOffset, FILE_BEGIN) );
	RETNOK( inFile->ReadArray(dirList, m_oDataHeader.iDirectoryCount) );

	// Read file entries
	file_info_t* fileList = new file_info_t[m_oDataHeader.iFileCount];
	memset(fileList, 0, m_oDataHeader.iFileCount * sizeof(file_info_t));
	RETNOK( inFile->Seek(m_iDataHeaderOffset + m_oDataHeader.iFileOffset, FILE_BEGIN) );
	for (int i = 0; i < m_oDataHeader.iFileCount; i++)
	{
		if (m_oFileHeader.iVersionMajor == 2)
		{
			RETNOK( inFile->ReadValue(fileList[i].iNameOffset) );
			RETNOK( inFile->ReadValue(fileList[i].iFlags32) );
			RETNOK( inFile->ReadValue(fileList[i].iDataOffset) );
			RETNOK( inFile->ReadValue(fileList[i].iDataLengthCompressed) );
			RETNOK( inFile->ReadValue(fileList[i].iDataLength) );
			fileList[i].iModificationTime = 0;
		}
		else // 4 or 5
		{
			RETNOK( inFile->ReadValue(fileList[i].iNameOffset) );
			RETNOK( inFile->ReadValue(fileList[i].iDataOffset) );
			RETNOK( inFile->ReadValue(fileList[i].iDataLengthCompressed) );
			RETNOK( inFile->ReadValue(fileList[i].iDataLength) );
			RETNOK( inFile->ReadValue(fileList[i].iModificationTime) );
			RETNOK( inFile->ReadValue(fileList[i].iFlags16) );
		}
	}

	// Process all entries to file tree
	RETNOK( ProcessDirEntry(dirList, fileList, 0, m_sStringBlob, m_oFileHeader.iDataOffset, m_vItems) );

	delete [] dirList;
	delete [] fileList;
	free(m_sStringBlob);
	
	m_pInputFile = inFile;
	m_bIsValid = true;
	return true;
}

bool CSgaFile::ExtractFile( int index, HANDLE outfile )
{
	const HWStorageItem &fileInfo = m_vItems[index];
	
	return false;
}
