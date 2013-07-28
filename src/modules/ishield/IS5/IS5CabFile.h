#ifndef IS5CabFile_h__
#define IS5CabFile_h__

#include "ISCabFile.h"

namespace IS5
{

#include "IS5_CabStructs.h"

class IS5CabFile : public ISCabFile
{
protected:
	CABHEADER m_Header;
	CABDESC* m_pCabDesc;
	DWORD* m_pDFT;

	std::vector<DWORD> m_vValidFiles;
	std::vector<FILEGROUPDESC> m_vFileGroups;
	std::vector<DWORD> m_vComponents;

	void GenerateInfoFile();
	bool IsVersionCompatible(DWORD version);
	bool InternalOpen(HANDLE headerFile);

public:	
	IS5CabFile();
	~IS5CabFile();
	
	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const;
};

} // namespace IS5

#endif // IS5CabFile_h__
