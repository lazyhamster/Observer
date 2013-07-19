#ifndef IS5CabFile_h__
#define IS5CabFile_h__

#include "ISCabFile.h"

namespace IS5
{

#include "IS5_CabStructs.h"

class IS5CabFile : public ISCabFile
{
private:
	HANDLE m_hHeaderFile;
	
	CABHEADER m_Header;
	CABDESC* m_pCabDesc;
	DWORD* m_pDFT;

	std::vector<DWORD> m_vValidFiles;
	std::vector<FILEGROUPDESC> m_vFileGroups;
	std::vector<DWORD> m_vComponents;

	std::wstring m_sInfoFile;

	void GenerateInfoFile();

public:	
	IS5CabFile();
	~IS5CabFile();
	
	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	const std::wstring GetCabInfo() const { return m_sInfoFile; }
	bool ExtractFile(int itemIndex, HANDLE targetFile) const;

	bool Open(HANDLE headerFile);
	void Close();
};

} // namespace IS5

#endif // IS5CabFile_h__
