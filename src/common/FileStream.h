#ifndef FileStream_h__
#define FileStream_h__

class CFileStream
{
protected:
	HANDLE m_hFile;
	wchar_t* m_strPath;
	bool m_fReadOnly;

	// Cache size value for read-only files
	int64_t m_nSizeCache;

public:
	CFileStream(const wchar_t* filePath, bool readOnly, bool createIfNotExists);
	~CFileStream();

	void Close();
	bool IsValid();

	int64_t GetSize();
	
	int64_t GetPos();
	bool SetPos(int64_t newPos);
	int64_t Seek(int64_t seekPos, int8_t seekOrigin);

	bool ReadBuffer(LPVOID buffer, size_t bufferSize);
	bool WriteBuffer(LPVOID buffer, size_t bufferSize);

	const wchar_t* FileName() const { return m_strPath; }
};

#endif // FileStream_h__
