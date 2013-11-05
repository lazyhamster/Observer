#ifndef SfFile_h__
#define SfFile_h__

#include "Streams.h"

struct SFFileEntry
{
	wchar_t LocalPath[MAX_PATH];
	int64_t UnpackedSize;
	int64_t PackedSize;
	bool IsCompressed;
	int64_t DataOffset;
	uint32_t CRC;
};

class SetupFactoryFile
{
protected:
	CFileStream* m_pInFile;
	std::vector<SFFileEntry> m_vFiles;

public:
	virtual bool Open(CFileStream* inFile) = 0;
	virtual void Close() = 0;
	virtual int EnumFiles() = 0;
	virtual int GetVersion() = 0;

	size_t GetCount() { return m_vFiles.size(); };
	SFFileEntry GetFile(int index) { return m_vFiles[index]; }

	virtual bool ExtractFile(int index, AStream* outStream) = 0;
};

SetupFactoryFile* OpenInstaller(const wchar_t* filePath);
void FreeInstaller(SetupFactoryFile* file);

#endif // SfFile_h__
