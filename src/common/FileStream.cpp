#include "stdafx.h"
#include "FileStream.h"

CFileStream::CFileStream( const wchar_t* filePath, bool readOnly, bool createIfNotExists )
{
	m_strPath = _wcsdup(filePath);
	m_fReadOnly = readOnly;
	m_nSizeCache = -1;
	
	DWORD dwAccess = readOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;
	DWORD dwDisposition = createIfNotExists && !readOnly ? OPEN_ALWAYS : OPEN_EXISTING;
	m_hFile = CreateFile(filePath, dwAccess, FILE_SHARE_READ, NULL, dwDisposition, FILE_ATTRIBUTE_NORMAL, 0);
}

CFileStream::~CFileStream()
{
	Close();
}

void CFileStream::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	if (m_strPath)
	{
		free(m_strPath);
		m_strPath = NULL;
	}
	m_nSizeCache = -1;
}

bool CFileStream::IsValid()
{
	return (m_hFile != INVALID_HANDLE_VALUE);
}

int64_t CFileStream::GetSize()
{
	if (!IsValid())
		return 0;

	if (m_fReadOnly && (m_nSizeCache > 0))
		return m_nSizeCache;
	
	LARGE_INTEGER liSize = {0};
	GetFileSizeEx(m_hFile, &liSize);
	m_nSizeCache = liSize.QuadPart;
	return liSize.QuadPart;
}

int64_t CFileStream::GetPos()
{
	return Seek(0, FILE_CURRENT);
}

bool CFileStream::SetPos( int64_t newPos )
{
	return Seek(newPos, FILE_BEGIN) == newPos;
}

int64_t CFileStream::Seek( int64_t seekPos, int8_t seekOrigin )
{
	LARGE_INTEGER newPos, resultPos;

	newPos.QuadPart = seekPos;
	BOOL retVal = SetFilePointerEx(m_hFile, newPos, &resultPos, seekOrigin);

	return resultPos.QuadPart;
}

bool CFileStream::ReadBuffer( LPVOID buffer, size_t bufferSize )
{
	if (!IsValid()) return false;

	if ((nullptr == buffer) || (UINT32_MAX < bufferSize)) return false;
	if (0 == bufferSize) return true;

	DWORD dwBytes;
	BOOL fReadResult = ReadFile(m_hFile, buffer, (DWORD) bufferSize, &dwBytes, NULL);

	return fReadResult && (dwBytes == bufferSize);
}

bool CFileStream::WriteBuffer( LPVOID buffer, size_t bufferSize )
{
	if (m_fReadOnly || !IsValid()) return false;

	if ((nullptr == buffer) || (UINT32_MAX < bufferSize)) return false;
	if (0 == bufferSize) return true;

	DWORD dwBytes;
	BOOL fWriteResult = WriteFile(m_hFile, buffer, (DWORD) bufferSize, &dwBytes, nullptr);

	return fWriteResult && (dwBytes == bufferSize);
}
