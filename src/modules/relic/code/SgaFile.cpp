#include "stdafx.h"
#include "SgaFile.h"
#include "SgaStructs.h"

#define RETNOK(x) if (!x) return false

static bool IsSupportedVersion(unsigned short major, unsigned short minor)
{
	return (minor == 0) && (major == 2 || major == 4 || major == 5);
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

	_file_header_raw_t header = {0};
	char *m_sStringBlob = NULL;
	long m_iDataHeaderOffset = 0;

	RETNOK( inFile->ReadExact(header.sIdentifier, 8) );
	RETNOK( strncmp(header.sIdentifier, "_ARCHIVE", 8) == 0 );
	
	RETNOK( inFile->ReadValue(header.iVersionMajor) );
	RETNOK( inFile->ReadValue(header.iVersionMinor) );
	RETNOK( IsSupportedVersion(header.iVersionMajor, header.iVersionMinor) );

	RETNOK( inFile->ReadArray(header.iContentsMD5, 4) );
	RETNOK( inFile->ReadArray(header.sArchiveName, 64) );
	RETNOK( inFile->ReadArray(header.iHeaderMD5, 4) );
	RETNOK( inFile->ReadValue(header.iDataHeaderSize) );
	RETNOK( inFile->ReadValue(header.iDataOffset) );

	if(header.iVersionMajor == 5)
	{
		RETNOK( inFile->ReadValue(m_iDataHeaderOffset) );
	}
	if(header.iVersionMajor >= 4)
	{
		RETNOK( inFile->ReadValue(header.iPlatform) );
		if(header.iPlatform != 1) return false;
	}
	else
	{
		header.iPlatform = 1;
	}

	if(header.iVersionMajor == 5)
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
	RETNOK( inFile->ReadValue(header.iEntryPointOffset) );
	RETNOK( inFile->ReadValue(header.iEntryPointCount) );
	RETNOK( inFile->ReadValue(header.iDirectoryOffset) );
	RETNOK( inFile->ReadValue(header.iDirectoryCount) );
	RETNOK( inFile->ReadValue(header.iFileOffset) );
	RETNOK( inFile->ReadValue(header.iFileCount) );
	RETNOK( inFile->ReadValue(header.iStringOffset) );
	RETNOK( inFile->ReadValue(header.iStringCount) );

	// Pre-load file and directory names
	RETNOK( inFile->Seek(m_iDataHeaderOffset + header.iStringOffset, FILE_BEGIN) );
	size_t iStringsLength = header.iDataHeaderSize - header.iStringOffset;
	m_sStringBlob = (char*) malloc(iStringsLength);
	RETNOK( inFile->ReadArray(m_sStringBlob, iStringsLength) );
	
	m_pInputFile = inFile;
	m_bIsValid = true;
	return true;
}

void CSgaFile::OnGetFileInfo( int index )
{

}

bool CSgaFile::ExtractFile( int index, HANDLE outfile )
{
	return false;
}
