#ifndef HWAbstractFile_h__
#define HWAbstractFile_h__

struct HWStorageItem
{
	wchar_t Name[MAX_PATH];
	uint32_t Flags;
	uint32_t CompressedSize;
	uint32_t UncompressedSize;
	FILETIME ModTime;
	uint32_t CRC;
	uint32_t DataOffset;
	int CustomData;
};

class CHWClassFactory;

class CHWAbstractStorage
{
	friend class CHWClassFactory;

protected:
	HANDLE m_hInputFile;
	std::vector<HWStorageItem> m_vItems;
	bool m_bIsValid;
	
	virtual bool Open(HANDLE inFile) = 0;
	virtual void Close() = 0;

	bool ReadData(void* buf, DWORD size);
	bool ReadData(void* buf, DWORD size, DWORD* numRead);
	bool SeekPos(__int64 newPos, DWORD moveMethod);

	virtual void OnGetFileInfo(int index) = 0;

public:
	CHWAbstractStorage() : m_hInputFile(INVALID_HANDLE_VALUE), m_bIsValid(false) {}
	virtual ~CHWAbstractStorage() {};
	
	int NumFiles() const { return (int) m_vItems.size(); }
	bool GetFileInfo(int index, HWStorageItem *item);
	bool IsValid() const { return m_bIsValid; }

	virtual const wchar_t* GetFormatName() const { return L"Generic"; }
	virtual const wchar_t* GetCompression() const { return L"None"; }

	virtual bool ExtractFile(int index, HANDLE outfile) = 0;
};

void UnixTimeToFileTime(time_t t, LPFILETIME pft);

#endif // HWAbstractFile_h__