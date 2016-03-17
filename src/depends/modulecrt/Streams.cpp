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

int64_t AStream::CopyFrom( AStream* src, int64_t maxBytes )
{
	const size_t copyBufSize = 32 * 1024;
	uint8_t copyBuf[copyBufSize];
	
	int64_t totalDataSize = min(src->GetSize() - src->GetPos(), maxBytes);
	int64_t bytesLeft = totalDataSize;
	while (bytesLeft > 0)
	{
		size_t copySize = (size_t) min(bytesLeft, copyBufSize);
		src->ReadBuffer(copyBuf, copySize);
		this->WriteBuffer(copyBuf, copySize);
		bytesLeft -= copySize;
	}

	return totalDataSize;
}

bool AStream::ReadBuffer( LPVOID buffer, size_t bufferSize )
{
	size_t readSize;
	return ReadBufferAny(buffer, bufferSize, &readSize) && (readSize == bufferSize);
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

CFileStream::CFileStream( HANDLE fileHandle, bool readOnly )
{
	m_strPath = nullptr;
	m_fReadOnly = readOnly;
	m_nSizeCache = -1;
	m_hFile = fileHandle;
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

bool CFileStream::ReadBufferAny( LPVOID buffer, size_t bufferSize, size_t *readSize )
{
	if (!IsValid()) return false;

	if ((nullptr == buffer) || (UINT32_MAX < bufferSize)) return false;
	if (0 == bufferSize) return true;

	DWORD dwBytes;
	BOOL fReadResult = ReadFile(m_hFile, buffer, (DWORD) bufferSize, &dwBytes, NULL);

	if (fReadResult && readSize)
	{
		*readSize = dwBytes;
	}

	return fReadResult != FALSE;
}

bool CFileStream::WriteBuffer( LPCVOID buffer, size_t bufferSize )
{
	if (m_fReadOnly || !IsValid()) return false;

	if ((nullptr == buffer) || (UINT32_MAX < bufferSize)) return false;
	if (0 == bufferSize) return true;

	DWORD dwBytes;
	BOOL fWriteResult = WriteFile(m_hFile, buffer, (DWORD) bufferSize, &dwBytes, nullptr);

	return fWriteResult && (dwBytes == bufferSize);
}

bool CFileStream::Clear()
{
	if (m_fReadOnly || !IsValid()) return false;
	
	return SetPos(0) && (SetEndOfFile(m_hFile) != 0);
}

CFileStream* CFileStream::Open( const wchar_t* filePath, bool readOnly, bool createIfNotExists )
{
	CFileStream* pStream = new CFileStream(filePath, readOnly, createIfNotExists);
	if (pStream->IsValid())	return pStream;

	delete pStream;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

bool CNullStream::ReadBufferAny( LPVOID buffer, size_t bufferSize, size_t *readSize )
{
	return false;
}

bool CNullStream::WriteBuffer( LPCVOID buffer, size_t bufferSize )
{
	return true;
}

bool CNullStream::Clear()
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
		if ((currPos + seekPos > m_nDataSize) || (currPos + seekPos < 0)) return false;
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

bool CMemoryStream::ReadBufferAny( LPVOID buffer, size_t bufferSize, size_t *readSize )
{
	if (!buffer || ((size_t)(m_pCurrPtr - m_pDataBuffer) >= m_nDataSize))
		return false;
	if (bufferSize == 0)
		return true;

	size_t copySize = min(bufferSize, m_nDataSize - (m_pCurrPtr - m_pDataBuffer));

	memcpy(buffer, m_pCurrPtr, copySize);
	m_pCurrPtr += copySize;
	if (readSize)
	{
		*readSize = copySize;
	}
	return true;
}

bool CMemoryStream::WriteBuffer( LPCVOID buffer, size_t bufferSize )
{
	if (!buffer) return false;
	if (bufferSize == 0) return true;

	size_t newSize = m_pCurrPtr - m_pDataBuffer + bufferSize;
	if (newSize > m_nCapacity)
	{
		size_t newCapacity = ((newSize / MEM_EXPAND_CHUNK_SIZE) + 1) * MEM_EXPAND_CHUNK_SIZE;
		SetCapacity(newCapacity);
	}

	memcpy(m_pCurrPtr, buffer, bufferSize);
	m_pCurrPtr += bufferSize;
	if (newSize > m_nDataSize)
	{
		m_nDataSize = newSize;
	}
	return true;
}

bool CMemoryStream::Clear()
{
	m_nDataSize = 0;
	m_pCurrPtr = m_pDataBuffer;

	return true;
}

bool CMemoryStream::Delete( size_t delSize )
{
	size_t bytesLeftFromPos = m_nDataSize - (m_pCurrPtr - m_pDataBuffer);
	size_t actualDelSize = min(delSize, bytesLeftFromPos);
	if (actualDelSize > 0)
	{
		memmove(m_pCurrPtr, m_pCurrPtr + actualDelSize, actualDelSize);
	}
	return true;
}

bool CMemoryStream::SetCapacity(size_t newCapacity)
{
	if (newCapacity != m_nCapacity)
	{
		size_t pos = min((size_t) (m_pCurrPtr - m_pDataBuffer), newCapacity);

		char* newBufferPtr = (char*) realloc(m_pDataBuffer, newCapacity);
		if (newBufferPtr != nullptr)
		{
			m_pDataBuffer = newBufferPtr;
			m_pCurrPtr = m_pDataBuffer + pos;
			m_nCapacity = newCapacity;
			m_nDataSize = min(m_nDataSize, m_nCapacity);
			
			return true;
		}
	}

	return false;
}

int64_t CMemoryStream::GetSize()
{
	return (int64_t) m_nDataSize;
}

//////////////////////////////////////////////////////////////////////////

CPartialStream::CPartialStream( AStream* parentStream, int64_t partStartOffset, int64_t partSize )
{
	m_pParentStream = parentStream;
	m_nStartOffset = max(partStartOffset, 0);
	m_nSize = max(partSize, 0);
	m_nCurrentPos = 0;
}

int64_t CPartialStream::GetSize()
{
	return m_nSize;
}

bool CPartialStream::Seek( int64_t seekPos, int8_t seekOrigin )
{
	return Seek(seekPos, nullptr, seekOrigin);
}

bool CPartialStream::Seek( int64_t seekPos, int64_t* newPos, int8_t seekOrigin )
{
	int64_t nextLocalPos = -1;
	
	switch (seekOrigin)
	{
	case STREAM_BEGIN:
		nextLocalPos = seekPos;
		break;
	case STREAM_CURRENT:
		nextLocalPos = nextLocalPos + seekPos;
		break;
	case STREAM_END:
		nextLocalPos = m_nSize - 1 + seekPos;
		break;
	}

	if (nextLocalPos < 0 || nextLocalPos >= m_nSize)
		return false;

	int64_t storedParentPos = m_pParentStream->GetPos();
	bool rval = m_pParentStream->SetPos(m_nStartOffset + nextLocalPos);
	m_pParentStream->SetPos(storedParentPos);

	if (rval)
	{
		m_nCurrentPos = nextLocalPos;
		if (newPos) *newPos = nextLocalPos;
	}

	return rval;
}

bool CPartialStream::ReadBufferAny( LPVOID buffer, size_t bufferSize, size_t *readSize )
{
	if ((nullptr == buffer) || (UINT32_MAX < bufferSize)) return false;
	if (bufferSize == 0) return true;

	int64_t storedParentPos = m_pParentStream->GetPos();
	size_t actualReadSize = 0;

	size_t copySize = min(bufferSize, (size_t) (m_nSize - m_nCurrentPos));
	m_pParentStream->SetPos(m_nStartOffset + m_nCurrentPos);
	bool rval = m_pParentStream->ReadBufferAny(buffer, copySize, &actualReadSize);
	if (rval)
	{
		m_nCurrentPos += actualReadSize;
		if (readSize) *readSize = actualReadSize;
	}

	m_pParentStream->SetPos(storedParentPos);
	return rval;
}

bool CPartialStream::WriteBuffer( LPCVOID buffer, size_t bufferSize )
{
	if (!buffer) return false;
	if (bufferSize == 0) return true;
	if (bufferSize > (size_t) (m_nSize - m_nCurrentPos)) return false;
	
	int64_t storedParentPos = m_pParentStream->GetPos();
	
	m_pParentStream->SetPos(m_nStartOffset + m_nCurrentPos);
	bool rval = m_pParentStream->WriteBuffer(buffer, bufferSize);
	if (rval)
	{
		m_nCurrentPos += bufferSize;
	}

	m_pParentStream->SetPos(storedParentPos);
	return rval;
}

bool CPartialStream::Clear()
{
	//TODO: implement
	return false;
}
