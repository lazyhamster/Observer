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

	/*
	_file_header_raw_t header;
	RETNOK( ReadData(&header, sizeof(header)) );

	if (strncmp(header.sIdentifier, "_ARCHIVE", 8) || !IsSupportedVersion(header.iVersionMajor, header.iVersionMinor))
		return false;
	*/
	
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
