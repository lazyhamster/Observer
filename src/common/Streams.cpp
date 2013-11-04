#include "stdafx.h"
#include "Streams.h"

int64_t AStream::GetPos()
{
	int64_t pos;
	return Seek(0, &pos, STREAM_CURRENT) ? pos : 0;
}

bool AStream::SetPos( int64_t newPos )
{
	int64_t resultPos;
	return Seek(newPos, &resultPos, STREAM_BEGIN) && (resultPos == newPos);
}

//////////////////////////////////////////////////////////////////////////

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

bool CFileStream::Seek( int64_t seekPos, int8_t seekOrigin )
{
	return Seek(seekPos, nullptr, seekOrigin);
}

bool CFileStream::Seek( int64_t seekPos, int64_t* newPos, int8_t seekOrigin )
{
	LARGE_INTEGER liNewPos, resultPos;

	liNewPos.QuadPart = seekPos;
	BOOL retVal = SetFilePointerEx(m_hFile, liNewPos, &resultPos, seekOrigin);

	if (newPos) *newPos = resultPos.QuadPart;
	return retVal != FALSE;
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

//////////////////////////////////////////////////////////////////////////

bool CNullStream::ReadBuffer( LPVOID buffer, size_t bufferSize )
{
	return false;
}

bool CNullStream::WriteBuffer( LPVOID buffer, size_t bufferSize )
{
	return true;
}

bool CNullStream::Seek( int64_t seekPos, int8_t seekOrigin )
{
	return false;
}

bool CNullStream::Seek( int64_t seekPos, int64_t* newPos, int8_t seekOrigin )
{
	return false;
}

int64_t CNullStream::GetSize()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////


CMemoryStream::CMemoryStream( size_t initialCapacity )
{
	m_nCapacity = initialCapacity;
	m_nDataSize = 0;
	m_pDataBuffer = (char*) malloc(initialCapacity);
	m_pCurrPtr = m_pDataBuffer;
}

CMemoryStream::~CMemoryStream()
{
	free(m_pDataBuffer);
}

bool CMemoryStream::Seek( int64_t seekPos, int8_t seekOrigin )
{
	return Seek(seekPos, nullptr, seekOrigin);
}

bool CMemoryStream::Seek( int64_t seekPos, int64_t* newPos, int8_t seekOrigin )
{
	size_t currPos = m_pCurrPtr - m_pDataBuffer;
	switch (seekOrigin)
	{
	case STREAM_BEGIN:
		if ((seekPos < 0) || (seekPos >= m_nDataSize)) return false;
		m_pCurrPtr = m_pDataBuffer + seekPos;
		break;
	case STREAM_CURRENT:
		if ((currPos + seekPos >= m_nDataSize) || (currPos + seekPos < 0)) return false;
		m_pCurrPtr += seekPos;
		break;
	case STREAM_END:
		if ((seekPos > 0) || (m_nDataSize + seekPos < 0)) return false;
		m_pCurrPtr = m_pDataBuffer + m_nDataSize + seekPos;
		break;
	}

	if (newPos)
	{
		*newPos = m_pCurrPtr - m_pDataBuffer;
	}

	return true;
}

bool CMemoryStream::ReadBuffer( LPVOID buffer, size_t bufferSize )
{
	if (!buffer || (m_pCurrPtr - m_pDataBuffer + bufferSize > m_nDataSize))
		return false;
	if (bufferSize == 0)
		return true;

	memcpy(buffer, m_pCurrPtr, bufferSize);
	m_pCurrPtr += bufferSize;
	return true;
}

bool CMemoryStream::WriteBuffer( LPVOID buffer, size_t bufferSize )
{
	if (!buffer) return false;
	if (bufferSize == 0) return true;

	size_t newSize = m_pCurrPtr - m_pDataBuffer + bufferSize;
	if (newSize > m_nCapacity)
	{
		size_t newCapacity = ((newSize / MEM_EXPAND_CHUNK_SIZE) + 1) * MEM_EXPAND_CHUNK_SIZE;
		size_t currPos = m_pCurrPtr - m_pDataBuffer;

		m_pDataBuffer = (char*) realloc(m_pDataBuffer, newCapacity);
		m_pCurrPtr = m_pDataBuffer + currPos;
		m_nCapacity = newCapacity;
	}

	memcpy(m_pCurrPtr, buffer, bufferSize);
	m_pCurrPtr += bufferSize;
	if (newSize > m_nDataSize)
	{
		m_nDataSize = newSize;
	}
	return true;
}

int64_t CMemoryStream::GetSize()
{
	return (int64_t) m_nDataSize;
}
