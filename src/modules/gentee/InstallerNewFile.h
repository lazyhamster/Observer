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
	GeaFile* ParentFile;
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
	GeaFile* m_pEmbedFiles;
	GeaFile* m_pMainFiles;

	bool ReadPESectionFiles(AStream* inStream, uint32_t scriptPackedSize, uint32_t dllPackedSize);

public:
	InstallerNewFile();
	~InstallerNewFile();
	
	virtual bool Open(AStream* inStream);
	virtual void Close();

	virtual int GetFilesCount() { return 0; }
	virtual bool GetFileDesc(int index, StorageItemInfo* desc);

	virtual bool ExtractFile(int index, AStream* dest);

	virtual void GetFileTypeName(wchar_t* buf, size_t bufSize);
	virtual void GetCompressionName(wchar_t* buf, size_t bufSize);
};

#endif // InstallerNewFile_h__
