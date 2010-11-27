#include "stdafx.h"
#include "SgaFile.h"

CSgaFile::CSgaFile()
{

}

CSgaFile::~CSgaFile()
{
	BaseClose();
}

bool CSgaFile::Open( HANDLE inFile )
{
	// If file is already open then exit
	if (inFile == INVALID_HANDLE_VALUE || m_bIsValid)
		return false;
	
	return false;
}

void CSgaFile::OnGetFileInfo( int index )
{

}

bool CSgaFile::ExtractFile( int index, HANDLE outfile )
{
	return false;
}
