#include "stdafx.h"
#include "HW1BigFile.h"
#include "HW1Decomp.h"

#define RETNOK(x) if (!x) return false

//////////////////////////// from bigfile.h  /////////////////////////////////////////

struct bigTOCFileEntry
{
	// together, the CRCs and nameLength make up a very unique identifier for
	// the filename (so we don't have to do string compares when searching or
	// or store long names in the TOC)
	uint32_t nameCRC1, nameCRC2;  // CRC of 1st and 2nd halves of name
	uint16_t nameLength;

	//char name[BF_MAX_FILENAME_LENGTH];  for speed & space concerns, these now precede the datafile portions of the bigfile

	uint32_t storedLength;
	uint32_t realLength;
	uint32_t offset;
	//time_t timeStamp;
	long timeStamp;
	char compressionType;  // 0/1
};

#define BF_FLAG_TOC_SORTED 1

struct bigTOC
{
	int numFiles;
	int flags;
	bigTOCFileEntry *fileEntries;
};

#define BIG_HEADER_NEED "RBF1.23"

//////////////////////////////////////////////////////////////////////////

static void bigFilenameDecrypt(char *filename, int len)
{
	int maskch = 213;
	while (len--)
	{
		*filename ^= maskch;
		maskch = *(filename++);
	}
}

//////////////////////////////////////////////////////////////////////////

CHW1BigFile::CHW1BigFile()
{
}

CHW1BigFile::~CHW1BigFile()
{
	BaseClose();
}

bool CHW1BigFile::Open( CBasicFile* inFile )
{
	// If file is already open then exit
	if (!inFile || !inFile->IsOpen() || m_bIsValid)
		return false;

	RETNOK( inFile->Seek(0, FILE_BEGIN) );

	// Check header
	char headerBytes[16] = {0};
	RETNOK( inFile->ReadExact(headerBytes, (DWORD) strlen(BIG_HEADER_NEED)) );
	if (strcmp(headerBytes, BIG_HEADER_NEED) != 0) return false;

	// Read TOC
	bigTOC toc = {0};
	RETNOK( inFile->ReadValue(toc.numFiles) );
	RETNOK( inFile->ReadValue(toc.flags) );

	size_t nMemSize = toc.numFiles * sizeof(bigTOCFileEntry);
	toc.fileEntries = (bigTOCFileEntry *) malloc(nMemSize);
	if (!inFile->ReadExact(toc.fileEntries, (DWORD) nMemSize))
	{
		free(toc.fileEntries);
		return false;
	}

	bool fResult = true;
	m_vItems.reserve(toc.numFiles);

	for (int i = 0; i < toc.numFiles; i++)
	{
		HWStorageItem item = {0};
		bigTOCFileEntry &fileEntry = toc.fileEntries[i];

		item.CustomData = fileEntry.nameLength;
		item.Flags = fileEntry.compressionType;
		item.UncompressedSize = fileEntry.realLength;
		item.CompressedSize = fileEntry.storedLength;
		item.DataOffset = fileEntry.offset + fileEntry.nameLength + 1;
		UnixTimeToFileTime(fileEntry.timeStamp, &item.ModTime);
		
		m_vItems.push_back(item);
	}
	
	free(toc.fileEntries);

	m_bIsValid = fResult;
	if (m_bIsValid)	m_pInputFile = inFile;

	return fResult;
}

bool CHW1BigFile::ExtractFile( int index, HANDLE outfile )
{
	HWStorageItem &item = m_vItems[index];
	RETNOK( m_pInputFile->Seek(item.DataOffset, FILE_BEGIN) );

	int nDataLen = item.UncompressedSize;
	char *buf = (char*) malloc(nDataLen);

	bool fOpResult = false;
	if (item.Flags == 1)
	{
		BIT_FILE *bitFile;
		int expandedSize, storedSize;

		// expand compressed file data directly into memory
		bitFile = bitioFileInputStart(m_pInputFile->RawHandle());
		expandedSize = lzssExpandFileToBuffer(bitFile, buf, nDataLen);
		storedSize = bitioFileInputStop(bitFile);
		fOpResult = (expandedSize == item.UncompressedSize) && (storedSize == item.CompressedSize);
	}
	else
	{
		fOpResult = m_pInputFile->ReadExact(buf, nDataLen);
	}

	if (fOpResult)
	{
		DWORD nWritten;
		fOpResult = WriteFile(outfile, buf, nDataLen, &nWritten, NULL) != FALSE;
	}

	free(buf);
	return fOpResult;
}

void CHW1BigFile::OnGetFileInfo( int index )
{
	HWStorageItem &item = m_vItems[index];
	if (item.Name[0]) return; // If name is already read then exit
	
	char szNameBuf[MAX_PATH] = {0};
	int nFileNameLen = item.CustomData;
	
	// Read file name
	bool fRetVal = m_pInputFile->Seek(item.DataOffset - nFileNameLen - 1, FILE_BEGIN)
		&& m_pInputFile->ReadExact(szNameBuf, nFileNameLen);

	bigFilenameDecrypt(szNameBuf, nFileNameLen);
	MultiByteToWideChar(CP_UTF8, 0, szNameBuf, -1, item.Name, MAX_PATH);
}
