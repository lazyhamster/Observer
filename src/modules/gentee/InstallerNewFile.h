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
	std::wstring m_sCompressionName;

	bool ReadPESectionFiles(AStream* inStream, uint32_t scriptPackedSize, uint32_t dllPackedSize);

public:
	InstallerNewFile();
	virtual ~InstallerNewFile();
	
	virtual bool Open(AStream* inStream, int64_t startOffset, bool ownStream);
	virtual void Close();

	virtual int GetFilesCount() { return (int) m_vFiles.size(); }
	virtual bool GetFileDesc(int index, StorageItemInfo* desc);

	virtual GenteeExtractResult ExtractFile(int index, AStream* dest, const char* password);

	virtual const wchar_t* GetFileTypeName() { return L"CreateInstall Installer"; }
	virtual const wchar_t* GetCompressionName() { return m_sCompressionName.c_str(); }
};

#endif // InstallerNewFile_h__
