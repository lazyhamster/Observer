#ifndef ISCabFile_h__
#define ISCabFile_h__

#include "ModuleDef.h"

#define CAB_EXTRACT_OK SER_SUCCESS
#define CAB_EXTRACT_READ_ERR SER_ERROR_READ
#define CAB_EXTRACT_WRITE_ERR SER_ERROR_WRITE
#define CAB_EXTRACT_USER_ABORT SER_USERABORT

class ISCabFile
{
protected:
	HANDLE m_hHeaderFile;
	std::wstring m_sCabPattern;
	std::wstring m_sInfoFile;
	
	virtual void GenerateInfoFile() = 0;
	virtual bool InternalOpen(HANDLE headerFile) = 0;

	int TransferFile(HANDLE src, HANDLE dest, __int64 fileSize, bool decrypt, BYTE* hashBuf, ExtractProcessCallbacks* progress);
	int UnpackFile(HANDLE src, HANDLE dest, __int64 unpackedSize, BYTE* hashBuf, ExtractProcessCallbacks* progress);

public:	
	virtual int GetTotalFiles() const = 0;
	virtual bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const = 0;
	virtual int ExtractFile(int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx) = 0;

	bool Open(HANDLE headerFile, const wchar_t* heaerFilePath);
	virtual void Close() = 0;

	const std::wstring GetCabInfo() const { return m_sInfoFile; }
	virtual bool HasInfoData() const = 0;
	virtual DWORD MajorVersion() const = 0;
};

ISCabFile* OpenCab(const wchar_t* filePath);
void CloseCab(ISCabFile* cab);

#endif // ISCabFile_h__
