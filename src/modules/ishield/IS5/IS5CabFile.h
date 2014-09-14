#ifndef IS5CabFile_h__
#define IS5CabFile_h__

#include "ISCabFile.h"

namespace IS5
{

#include "IS5_CabStructs.h"

struct DataVolume
{
	DWORD VolumeIndex;
	std::wstring FilePath;
	CFileStream* FileHandle;
	__int64 FileSize;
	CABHEADER Header;
};

enum CompressionSystem
{
	Unknown,
	New,
	Old
};

class IS5CabFile : public ISCabFile
{
protected:
	CABHEADER m_Header;
	CABDESC* m_pCabDesc;
	DWORD* m_pDFT;

	CompressionSystem m_eCompression;

	std::vector<DWORD> m_vValidFiles;
	std::vector<FILEGROUPDESC> m_vFileGroups;
	std::vector<DWORD> m_vComponents;

	std::vector<DataVolume*> m_vVolumes;

	void GenerateInfoFile();
	bool IsVersionCompatible(DWORD version);
	bool InternalOpen(CFileStream* headerFile);

	DataVolume* OpenVolume(DWORD volumeIndex);

	void DetectCompression(FILEDESC* fileDesc, DataVolume* fileVolume);

public:	
	IS5CabFile();
	~IS5CabFile();
	
	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, CFileStream* targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const;
	bool HasInfoData() const { return true; }
	const wchar_t* GetCompression() const { return L"ZData"; }
};

} // namespace IS5

#endif // IS5CabFile_h__
