#ifndef ISCabFile_h__
#define ISCabFile_h__

#include "ModuleDef.h"

class ISCabFile
{
public:	
	virtual int GetTotalFiles() const = 0;
	virtual bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const = 0;
	virtual const std::wstring GetCabInfo() const = 0;
	virtual bool ExtractFile(int itemIndex, HANDLE targetFile) const = 0;

	virtual void Close() = 0;
};

ISCabFile* OpenCab(const wchar_t* filePath);
void CloseCab(ISCabFile* cab);

#endif // ISCabFile_h__
