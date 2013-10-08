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
	bool CheckManifest();
	__int64 ReadAnsiEntry(HANDLE hFile, SfxFilePlainEntry *pEntry);
	__int64 ReadUnicodeEntry(HANDLE hFile, SfxFilePlainEntry *pEntry);

protected:
	std::vector<SfxFilePlainEntry> m_vFiles;
	
	void GenerateInfoFile() { /* Not used for this class */ }
	bool InternalOpen(HANDLE headerFile);

public:
	ISPlainSfxFile();
	~ISPlainSfxFile();

	int GetTotalFiles() const;
	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
	int ExtractFile(int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx);

	void Close();

	DWORD MajorVersion() const { return -1; }
	bool HasInfoData() const { return false; }
};

} // namespace ISSfx

#endif // ISPlainSfxFile_h__
