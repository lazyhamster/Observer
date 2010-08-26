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

	template<typename T>
	bool ReadStructArray(BIG_SectionRef secref, T** list);

	bool FetchFolder(BIG_FolderListEntry* folders, int folderIndex, BIG_FileInfoListEntry* files,
		const char* prefix, const char* namesBuf);
	bool ExtractPlain(BIG_FileInfoListEntry &fileInfo, HANDLE outfile, uint32_t fileCRC);
	bool ExtractCompressed(BIG_FileInfoListEntry &fileInfo, HANDLE outfile, uint32_t fileCRC);

protected:
	virtual bool Open(HANDLE inFile);
	virtual void Close();

	virtual void OnGetFileInfo(int index) {}

public:
	CHW2BigFile();
	virtual ~CHW2BigFile();

	virtual const wchar_t* GetFormatName() const { return L"HomeWorld 2 BIG"; }
	virtual const wchar_t* GetCompression() const { return L"ZLib/None"; }

	virtual bool ExtractFile(int index, HANDLE outfile);
};

template<typename T>
bool CHW2BigFile::ReadStructArray( BIG_SectionRef secref, T** list )
{
	if (secref.Count > 10000) return false;
		
	if (!SeekPos(secref.Offset + sizeof(BIG_ArchHeader), FILE_BEGIN))
		return false;

	size_t nMemSize = secref.Count * sizeof(T);
	T* data = (T*) malloc(nMemSize);
	if (data == NULL) return false;
	
	bool fOpRes = ReadData(data, nMemSize);
	if (fOpRes)
		*list = data;
	else
		free(data);
	
	return fOpRes;
}

#endif // HW2BigFile_h__