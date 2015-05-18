#ifndef ISCabFile_h__
#define ISCabFile_h__

#include "ModuleDef.h"
#include "modulecrt/Streams.h"

#define CAB_EXTRACT_OK SER_SUCCESS
#define CAB_EXTRACT_READ_ERR SER_ERROR_READ
#define CAB_EXTRACT_WRITE_ERR SER_ERROR_WRITE
#define CAB_EXTRACT_USER_ABORT SER_USERABORT

class ISCabFile
{
protected:
	CFileStream* m_pHeaderFile;
	std::wstring m_sCabPattern;
	std::wstring m_sInfoFile;
	
	virtual void GenerateInfoFile() = 0;
	virtual bool InternalOpen(CFileStream* headerFile) = 0;

	int TransferFile(CFileStream* src, CFileStream* dest, __int64 fileSize, bool decrypt, BYTE* hashBuf, ExtractProcessCallbacks* progress);
	int UnpackFile(CFileStream* src, CFileStream* dest, __int64 unpackedSize, BYTE* hashBuf, ExtractProcessCallbacks* progress);
	int UnpackFileOld(CFileStream* src, DWORD packedSize, CFileStream* dest, DWORD unpackedSize, BYTE* hashBuf, ExtractProcessCallbacks* progress);

public:	
	virtual ~ISCabFile() {}
	
	virtual int GetTotalFiles() const = 0;
	virtual bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const = 0;
	virtual int ExtractFile(int itemIndex, CFileStream* targetFile, ExtractProcessCallbacks progressCtx) = 0;

	bool Open(CFileStream* headerFile);
	virtual void Close() = 0;

	const std::wstring GetCabInfo() const { return m_sInfoFile; }
	virtual bool HasInfoData() const = 0;
	virtual DWORD MajorVersion() const = 0;
	virtual const wchar_t* GetCompression() const = 0;
};

ISCabFile* OpenCab(const wchar_t* filePath);
void CloseCab(ISCabFile* cab);

#endif // ISCabFile_h__
