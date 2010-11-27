#include "stdafx.h"
#include "BasicFile.h"

CBasicFile::CBasicFile( const wchar_t* path )
{
	m_wszPath = (path != NULL) ? _wcsdup(path) : NULL;
	m_hFile = INVALID_HANDLE_VALUE;
	m_nFileSize = 0;
}

CBasicFile::~CBasicFile()
{
	Close();
}

bool CBasicFile::Open()
{
	if (!m_wszPath) return false;
	if (IsOpen()) return true;

	m_hFile = CreateFile(m_wszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER liSize = {0};
		GetFileSizeEx(m_hFile, &liSize);
		m_nFileSize = liSize.QuadPart;

		return true;
	}
	
	return false;
}

void CBasicFile::Close()
{
	if (IsOpen())
	{
		CloseHandle(m_hFile);
		m_nFileSize = 0;
	}

	if (m_wszPath != NULL)
	{
		free(m_wszPath);
		m_wszPath = NULL;
	}
}

bool CBasicFile::Seek( __int64 position, DWORD origin )
{
	if (!IsOpen()) return false;
	
	LARGE_INTEGER nMovePos, nNewPos;
	
	nMovePos.QuadPart = position;
	BOOL ret = SetFilePointerEx(m_hFile, nMovePos, &nNewPos, origin);
	return (ret != FALSE);
}

DWORD CBasicFile::Read( void *buffer, DWORD readSize )
{
	if (!IsOpen()) return 0;

	DWORD bytesRead;
	BOOL ret = ReadFile(m_hFile, buffer, readSize, &bytesRead, NULL);
	return (ret == TRUE) ? bytesRead : 0;
}

bool CBasicFile::ReadExact( void *buffer, DWORD readSize )
{
	DWORD dataSize = Read(buffer, readSize);
	return (dataSize == readSize);
}

bool CBasicFile::ReadArray( void *buffer, DWORD itemSize, DWORD numItems )
{
	DWORD numBytes = itemSize * numItems;
	return ReadExact(buffer, numBytes);
}
