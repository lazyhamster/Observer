#ifndef InstallerNewFile_h__
#define InstallerNewFile_h__

#include "BaseGenteeFile.h"
#include "GeaFile.h"

struct InstallerFileDesc
{
	std::wstring Path;
	std::wstring PathPrefix; // used with ParentFile
	int64_t PackedSize;

	// Content and ParentFile are mutually exclusive
	AStream* Content;
	BaseGenteeFile* ParentFile;
	int ParentFileIndex;

	InstallerFileDesc() : PackedSize(0), Content(nullptr), ParentFile(nullptr), ParentFileIndex(-1)	{}

	~InstallerFileDesc()
	{
		if (Content) delete Content;
	}
};

class InstallerNewFile : public BaseGenteeFile
{
private:
	std::vector<InstallerFileDesc*> m_vFiles;
	BaseGenteeFile* m_pEmbedFiles;
	BaseGenteeFile* m_pMainFiles;

	bool ReadPESectionFiles(AStream* inStream, uint32_t scriptPackedSize, uint32_t dllPackedSize);

public:
	InstallerNewFile();
	virtual ~InstallerNewFile();
	
	virtual bool Open(AStream* inStream, int64_t startOffset, bool ownStream);
	virtual void Close();

	virtual int GetFilesCount() { return 0; }
	virtual bool GetFileDesc(int index, StorageItemInfo* desc);

	virtual GenteeExtractResult ExtractFile(int index, AStream* dest, const char* password);

	virtual void GetFileTypeName(wchar_t* buf, size_t bufSize);
	virtual void GetCompressionName(wchar_t* buf, size_t bufSize);
};

#endif // InstallerNewFile_h__
