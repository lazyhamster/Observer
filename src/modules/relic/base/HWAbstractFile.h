#ifndef HWAbstractFile_h__
#define HWAbstractFile_h__

#include "BasicFile.h"

struct HWStorageItem
{
	wchar_t Name[MAX_PATH];
	uint32_t Flags;
	uint32_t CompressedSize;
	uint32_t UncompressedSize;
	time_t ModTime;
	uint32_t CRC;
	uint32_t DataOffset;
	int CustomData;
};

class CHWClassFactory;

class CHWAbstractStorage
{
	friend class CHWClassFactory;

protected:
	CBasicFile* m_pInputFile;
	std::vector<HWStorageItem> m_vItems;
	bool m_bIsValid;
	
	virtual bool Open(CBasicFile* inFile) = 0;

	void BaseClose();

	virtual void OnGetFileInfo(int index) = 0;

public:
	CHWAbstractStorage() : m_pInputFile(NULL), m_bIsValid(false) {}
	virtual ~CHWAbstractStorage() {};
	
	int NumFiles() const { return (int) m_vItems.size(); }
	bool GetFileInfo(int index, HWStorageItem *item);
	bool IsValid() const { return m_bIsValid; }

	virtual const wchar_t* GetFormatName() const { return L"Generic"; }
	virtual const wchar_t* GetCompression() const { return L"None"; }
	virtual const wchar_t* GetLabel() const { return L"-"; }

	virtual bool ExtractFile(int index, HANDLE outfile) = 0;
};

#endif // HWAbstractFile_h__