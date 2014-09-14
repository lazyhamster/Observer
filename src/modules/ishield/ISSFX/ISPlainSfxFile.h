#ifndef ISPlainSfxFile_h__
#define ISPlainSfxFile_h__

#include "ISCabFile.h"

namespace ISSfx
{

struct SfxFilePlainEntry
{
	wchar_t Path[MAX_PATH];
	__int64 Size;
	__int64 StartOffset;
};

class ISPlainSfxFile : public ISCabFile
{
private:
	__int64 ReadAnsiEntry(CFileStream* pFile, SfxFilePlainEntry *pEntry);
	__int64 ReadUnicodeEntry(CFileStream* pFile, SfxFilePlainEntry *pEntry);

protected:
	std::vector<SfxFilePlainEntry> m_vFiles;
	
	void GenerateInfoFile() { /* Not used for this class */ }
	bool InternalOpen(CFileStream* headerFile);

public:
	ISPlainSfxFile();
	~ISPlainSfxFile();

	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, CFileStream* targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const { return -1; }
	bool HasInfoData() const { return false; }
	const wchar_t* GetCompression() const { return L"None"; }
};

} // namespace ISSfx

#endif // ISPlainSfxFile_h__
