#ifndef IS6CabFile_h__
#define IS6CabFile_h__

#include "ISCabFile.h"

namespace IS6
{

#include "IS6_CabStructs.h"

class IS6CabFile : public ISCabFile
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
	IS6CabFile();
	~IS6CabFile();

	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	const std::wstring GetCabInfo() const { return m_sInfoFile; }
	bool ExtractFile(int itemIndex, HANDLE targetFile) const;

	bool Open(HANDLE headerFile);
	void Close();
};

} // namespace IS5

#endif // IS6CabFile_h__
