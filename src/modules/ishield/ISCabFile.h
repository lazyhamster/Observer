#ifndef ISCabFile_h__
#define ISCabFile_h__

#include "ModuleDef.h"

#define CAB_EXTRACT_OK SER_SUCCESS
#define CAB_EXTRACT_READ_ERR SER_ERROR_READ
#define CAB_EXTRACT_WRITE_ERR SER_ERROR_WRITE

class ISCabFile
{
protected:
	HANDLE m_hHeaderFile;
	std::wstring m_sCabPattern;
	std::wstring m_sInfoFile;
	
	virtual bool IsVersionCompatible(DWORD version) = 0;
	virtual void GenerateInfoFile() = 0;
	virtual bool InternalOpen(HANDLE headerFile) = 0;

public:	
	virtual int GetTotalFiles() const = 0;
	virtual bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const = 0;
	virtual int ExtractFile(int itemIndex, HANDLE targetFile) = 0;

	bool Open(HANDLE headerFile, const wchar_t* heaerFilePath);
	virtual void Close() = 0;

	const std::wstring GetCabInfo() const { return m_sInfoFile; }
	virtual DWORD MajorVersion() const = 0;
};

ISCabFile* OpenCab(const wchar_t* filePath);
void CloseCab(ISCabFile* cab);

#endif // ISCabFile_h__
