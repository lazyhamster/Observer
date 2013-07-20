#ifndef IS5CabFile_h__
#define IS5CabFile_h__

#include "ISCabFile.h"

namespace IS5
{

#include "IS5_CabStructs.h"

class IS5CabFile : public ISCabFile
{
protected:
	HANDLE m_hHeaderFile;
	
	CABHEADER m_Header;
	CABDESC* m_pCabDesc;
	DWORD* m_pDFT;

	std::vector<DWORD> m_vValidFiles;
	std::vector<FILEGROUPDESC> m_vFileGroups;
	std::vector<DWORD> m_vComponents;

	void GenerateInfoFile();
	bool IsVersionCompatible(DWORD version);

public:	
	IS5CabFile();
	~IS5CabFile();
	
	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	bool ExtractFile(int itemIndex, HANDLE targetFile) const;

	bool Open(HANDLE headerFile);
	void Close();
};

} // namespace IS5

#endif // IS5CabFile_h__
