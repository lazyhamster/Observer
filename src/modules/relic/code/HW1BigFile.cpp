#include "stdafx.h"
#include "HW1BigFile.h"

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
	Close();
}

bool CHW1BigFile::Open( HANDLE inFile )
{
	Close();

	if (inFile == INVALID_HANDLE_VALUE)
		return false;

	LARGE_INTEGER nInputFileSize;
	RETNOK( GetFileSizeEx(inFile, &nInputFileSize) );

	m_hInputFile = inFile;
	RETNOK( SeekPos(0, FILE_BEGIN) );

	// Check header
	char headerBytes[16] = {0};
	RETNOK( ReadData(headerBytes, strlen(BIG_HEADER_NEED)) );
	if (strcmp(headerBytes, BIG_HEADER_NEED) != 0) return false;

	// Read TOC
	bigTOC toc = {0};
	RETNOK( ReadData(&toc.numFiles, sizeof(toc.numFiles)) );
	RETNOK( ReadData(&toc.flags, sizeof(toc.flags)) );

	size_t nMemSize = toc.numFiles * sizeof(bigTOCFileEntry);
	toc.fileEntries = (bigTOCFileEntry *) malloc(nMemSize);
	if (!ReadData(toc.fileEntries, nMemSize))
	{
		free(toc.fileEntries);
		return false;
	}

	bool fResult = true;
	m_vItems.reserve(toc.numFiles);

	for (int i = 0; i < toc.numFiles; i++)
	{
		HWStorageItem item = {0};
		char szNameBuf[MAX_PATH] = {0};

		bigTOCFileEntry &fileEntry = toc.fileEntries[i];

		item.CustomData = fileEntry.compressionType;
		item.UncompressedSize = fileEntry.realLength;
		item.CompressedSize = fileEntry.storedLength;
		item.DataOffset = fileEntry.offset;
		UnixTimeToFileTime(fileEntry.timeStamp, &item.ModTime);

		// Read file name
		fResult = SeekPos(fileEntry.offset, FILE_BEGIN) && ReadData(szNameBuf, fileEntry.nameLength);
		if (!fResult) break;

		bigFilenameDecrypt(szNameBuf, fileEntry.nameLength);
		MultiByteToWideChar(CP_UTF8, 0, szNameBuf, fileEntry.nameLength, item.Name, MAX_PATH);
		
		m_vItems.push_back(item);
	}
	
	free(toc.fileEntries);

	m_bIsValid = fResult;
	return fResult;
}

void CHW1BigFile::Close()
{
	if (m_bIsValid && m_hInputFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hInputFile);
		m_hInputFile = INVALID_HANDLE_VALUE;
	}

	m_vItems.clear();
	m_bIsValid = false;
}

bool CHW1BigFile::ExtractFile( int index, HANDLE outfile )
{
	return false;
}
