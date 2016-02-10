#ifndef InstallerOldFile_h__
#define InstallerOldFile_h__

#include "BaseGenteeFile.h"

class InstallerOldFile : public BaseGenteeFile
{
public:
	InstallerOldFile();
	virtual ~InstallerOldFile();

	virtual bool Open(AStream* inStream, int64_t startOffset, bool ownStream);
	virtual void Close();

	virtual int GetFilesCount() { return 0; }
	virtual bool GetFileDesc(int index, StorageItemInfo* desc);

	virtual GenteeExtractResult ExtractFile(int index, AStream* dest, const char* password);

	virtual const wchar_t* GetFileTypeName() { return L"Gentee Installer"; }
	virtual const wchar_t* GetCompressionName() { return L"Unknown"; }
};

#endif // InstallerOldFile_h__
