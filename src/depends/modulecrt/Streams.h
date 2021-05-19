#ifndef FileStream_h__
#define FileStream_h__

#include <stdint.h>

#define STREAM_BEGIN FILE_BEGIN
#define STREAM_CURRENT FILE_CURRENT
#define STREAM_END FILE_END

#define MAX_SIZE_INFINITE ULONG_MAX

class AStream
{
public:
	virtual ~AStream() {}
	
	virtual int64_t GetPos();
	virtual bool SetPos(int64_t newPos);
	bool Skip(int64_t numBytes) { return Seek(numBytes, STREAM_CURRENT); }

	virtual int64_t GetSize() = 0;
	virtual bool Seek(int64_t seekPos, int8_t seekOrigin) = 0;
	virtual bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin) = 0;
	virtual bool ReadBufferAny(LPVOID buffer, size_t bufferSize, size_t *readSize) = 0;
	virtual bool WriteBuffer(LPCVOID buffer, size_t bufferSize) = 0;
	virtual bool Clear() = 0;

	bool ReadBuffer(LPVOID buffer, size_t bufferSize);

	int64_t CopyFrom(AStream* src, int64_t maxBytes = MAX_SIZE_INFINITE);
	
	template <typename T>
	bool Read(T* val)
	{
		return ReadBuffer(val, sizeof(T));
	}
};

class CFileStream : public AStream
{
private:
	CFileStream() = delete;
	CFileStream(const CFileStream& other) = delete;
	CFileStream& operator=(const CFileStream& rhs) = delete;

protected:
	HANDLE m_hFile;
	wchar_t* m_strPath;
	bool m_fReadOnly;

	// Cache size value for read-only files
	int64_t m_nSizeCache;

public:
	CFileStream(const wchar_t* filePath, bool readOnly, bool createIfNotExists);
	CFileStream(HANDLE fileHandle, bool readOnly);
	~CFileStream();

	static CFileStream* Open(const wchar_t* filePath, bool readOnly, bool createIfNotExists);

	void Close();
	bool IsValid();

	int64_t GetSize();
	bool Seek(int64_t seekPos, int8_t seekOrigin);
	bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin);
	bool ReadBufferAny(LPVOID buffer, size_t bufferSize, size_t *readSize);
	bool WriteBuffer(LPCVOID buffer, size_t bufferSize);
	bool Clear();

	const wchar_t* FilePath() const { return m_strPath; }
	HANDLE GetHandle() { return m_hFile; }
};

#define MEM_EXPAND_CHUNK_SIZE (16 * 1024)

class CMemoryStream : public AStream
{
private:
	CMemoryStream(const CMemoryStream& other) {}
	CMemoryStream& operator=( const CMemoryStream& rhs ) {}

protected:
	char* m_pDataBuffer;
	size_t m_nCapacity;
	size_t m_nDataSize;
	char* m_pCurrPtr;

public:
	CMemoryStream(size_t initialCapacity = MEM_EXPAND_CHUNK_SIZE);
	~CMemoryStream();

	int64_t GetSize();
	bool Seek(int64_t seekPos, int8_t seekOrigin);
	bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin);
	bool ReadBufferAny(LPVOID buffer, size_t bufferSize, size_t *readSize);
	bool WriteBuffer(LPCVOID buffer, size_t bufferSize);
	bool Clear();

	bool Delete(size_t delSize);
	bool SetCapacity(size_t newCapacity);
	const char* CDataPtr() const { return m_pDataBuffer; }
};

class CNullStream : public AStream
{
public:
	CNullStream() {}

	int64_t GetSize();
	bool Seek(int64_t seekPos, int8_t seekOrigin);
	bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin);
	bool ReadBufferAny(LPVOID buffer, size_t bufferSize, size_t *readSize);
	bool WriteBuffer(LPCVOID buffer, size_t bufferSize);
	bool Clear();
};

class CPartialStream : public AStream
{
private:
	AStream* m_pParentStream;
	int64_t m_nStartOffset;
	int64_t m_nSize;
	int64_t m_nCurrentPos;

public:
	CPartialStream(AStream* parentStream, int64_t partStartOffset, int64_t partSize);

	int64_t GetSize();
	bool Seek(int64_t seekPos, int8_t seekOrigin);
	bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin);
	bool ReadBufferAny(LPVOID buffer, size_t bufferSize, size_t *readSize);
	bool WriteBuffer(LPCVOID buffer, size_t bufferSize);
	bool Clear();

	AStream* GetParentStream() { return m_pParentStream; }
};

#endif // FileStream_h__
