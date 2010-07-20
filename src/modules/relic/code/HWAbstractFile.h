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

	bool ReadData(void* buf, size_t size);
	bool SeekPos(__int64 newPos, DWORD moveMethod);

public:
	CHWAbstractStorage() : m_hInputFile(INVALID_HANDLE_VALUE), m_bIsValid(false) {}
	virtual ~CHWAbstractStorage() {};
	
	int NumFiles() { return m_vItems.size(); }
	bool GetFileInfo(int index, HWStorageItem *item);
	bool IsValid() { return m_bIsValid; }
};

#endif // HWAbstractFile_h__