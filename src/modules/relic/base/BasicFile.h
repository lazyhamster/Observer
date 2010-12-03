#ifndef BasicFile_h__
#define BasicFile_h__

class CBasicFile
{
private:
	wchar_t* m_wszPath;
	HANDLE m_hFile;
	__int64 m_nFileSize;

public:
	CBasicFile(const wchar_t* path);
	~CBasicFile();

	bool Open();
	void Close();

	bool Seek(__int64 position, DWORD origin);
	DWORD Read(void *buffer, DWORD readSize);

	bool ReadExact(void *buffer, DWORD readSize);
	
	template <typename T>
	bool ReadArray(T* buffer, DWORD numItems)
	{
		DWORD readSize = numItems * sizeof(T);
		return ReadExact(buffer, readSize);
	}

	template <typename T>
	bool ReadValue(T& dest)
	{
		return ReadExact(&dest, sizeof(T));
	}

	__int64 GetSize() { return m_nFileSize; }
	bool IsOpen() { return (m_hFile != INVALID_HANDLE_VALUE); }
	HANDLE RawHandle() { return m_hFile; }

	__int64 GetPosition();
};

#endif // BasicFile_h__