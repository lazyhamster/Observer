#ifndef HW2BigFile_h__
#define HW2BigFile_h__

#include "HWAbstractFile.h"
#include "HW2Structs.h"

class CHW2BigFile : public CHWAbstractStorage
{
private:
	BIG_ArchHeader m_archHeader;
	BIG_SectionHeader m_sectionHeader;
	BIG_FileInfoListEntry* m_vFileInfoList;

	bool FetchFolder(CBasicFile* inFile, BIG_FolderListEntry* folders, int folderIndex, BIG_FileInfoListEntry* files,
		const char* prefix, const char* namesBuf);
	bool ExtractPlain(BIG_FileInfoListEntry &fileInfo, HANDLE outfile, uint32_t fileCRC);
	bool ExtractCompressed(BIG_FileInfoListEntry &fileInfo, HANDLE outfile, uint32_t fileCRC);

protected:
	virtual bool Open(CBasicFile* inFile);

	virtual void OnGetFileInfo(int index) {}

public:
	CHW2BigFile();
	virtual ~CHW2BigFile();

	virtual const wchar_t* GetFormatName() const { return L"HomeWorld 2 BIG"; }
	virtual const wchar_t* GetCompression() const { return L"ZLib/None"; }
	virtual const wchar_t* GetLabel() const { return m_archHeader.name; }

	virtual bool ExtractFile(int index, HANDLE outfile);
};

#endif // HW2BigFile_h__