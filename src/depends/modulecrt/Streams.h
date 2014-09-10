#ifndef FileStream_h__
#define FileStream_h__

#define STREAM_BEGIN FILE_BEGIN
#define STREAM_CURRENT FILE_CURRENT
#define STREAM_END FILE_END

class AStream
{
public:
	virtual ~AStream() {}
	
	virtual int64_t GetPos();
	virtual bool SetPos(int64_t newPos);

	virtual int64_t GetSize() = 0;
	virtual bool Seek(int64_t seekPos, int8_t seekOrigin) = 0;
	virtual bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin) = 0;
	virtual bool ReadBuffer(LPVOID buffer, size_t bufferSize) = 0;
	virtual bool WriteBuffer(LPVOID buffer, size_t bufferSize) = 0;

	int64_t CopyFrom(AStream* src);
	
	template <typename T>
	bool Read(T* val)
	{
		return ReadBuffer(val, sizeof(T));
	}
};

class CFileStream : public AStream
{
private:
	CFileStream() {}
	CFileStream(const CFileStream& other) {}
	CFileStream& operator=( const CFileStream& rhs ) {}

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
	bool ReadBuffer(LPVOID buffer, size_t bufferSize);
	bool WriteBuffer(LPVOID buffer, size_t bufferSize);

	const wchar_t* FileName() const { return m_strPath; }
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
	bool ReadBuffer(LPVOID buffer, size_t bufferSize);
	bool WriteBuffer(LPVOID buffer, size_t bufferSize);

	const char* DataPtr() { return m_pDataBuffer; }
};

class CNullStream : public AStream
{
public:
	CNullStream() {}

	int64_t GetSize();
	bool Seek(int64_t seekPos, int8_t seekOrigin);
	bool Seek(int64_t seekPos, int64_t* newPos, int8_t seekOrigin);
	bool ReadBuffer(LPVOID buffer, size_t bufferSize);
	bool WriteBuffer(LPVOID buffer, size_t bufferSize);
};

#endif // FileStream_h__
