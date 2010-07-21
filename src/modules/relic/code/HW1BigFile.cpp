#include "stdafx.h"
#include "HW1BigFile.h"

#define RETNOK(x) if (!x) return false

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

	bool fResult = false;
	//TODO: implement

	m_bIsValid = fResult;
	return fResult;
}

void CHW1BigFile::Close()
{
	if (m_hInputFile != INVALID_HANDLE_VALUE)
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
