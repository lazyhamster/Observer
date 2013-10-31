#ifndef SfFile_h__
#define SfFile_h__

#include "FileStream.h"

struct SFFileEntry
{
	wchar_t LocalPath[MAX_PATH];
	int64_t UnpackedSize;
	int64_t PackedSize;
	bool IsCompressed;
	int64_t DataOffset;
};

class SetupFactoryFile
{
protected:
	CFileStream* m_pInFile;
	std::vector<SFFileEntry> m_vFiles;

public:
	virtual bool Open(CFileStream* inFile) = 0;
	virtual void Close() = 0;
};

SetupFactoryFile* OpenInstaller(const wchar_t* filePath);
void FreeInstaller(SetupFactoryFile* file);

#endif // SfFile_h__
