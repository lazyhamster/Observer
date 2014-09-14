#ifndef IS3CabFile_h__
#define IS3CabFile_h__

#include "ISCabFile.h"

namespace IS3
{

#include "IS3_Structs.h"

struct DirEntry
{
	WORD NumFiles;
	char Name[MAX_PATH];
};

class IS3CabFile : public ISCabFile
{
protected:
	CABHEADER m_Header;
	LPVOID m_pFileList;
	std::vector<DirEntry> m_vDirs;
	std::vector<FILEDESC*> m_vFiles;
	
	void GenerateInfoFile() { /* Not used for this class */ }
	bool InternalOpen(CFileStream* headerFile);

public:
	IS3CabFile();
	~IS3CabFile();

	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, CFileStream* targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const { return 3; }
	bool HasInfoData() const { return false; }
	const wchar_t* GetCompression() const { return L"PKWare DCL"; }
};

} // namespace IS3

#endif // IS3CabFile_h__
