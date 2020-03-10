#ifndef SfFile_h__
#define SfFile_h__

#include "modulecrt/Streams.h"

enum EntryCompression
{
	COMP_UNKNOWN = -1,
	COMP_NONE = 0,
	COMP_PKWARE = 1,
	COMP_LZMA = 2,
	COMP_LZMA2 = 3
};

struct SFFileEntry
{
	std::string LocalPath;
	int64_t UnpackedSize;
	int64_t PackedSize;
	EntryCompression Compression;
	int64_t DataOffset;
	int8_t Attributes;
	uint32_t CRC;
	time_t LastWriteTime;
	time_t CreationTime;
	bool IsXORed;

	SFFileEntry() : UnpackedSize(0), PackedSize(0),
		Compression(COMP_UNKNOWN), DataOffset(0), Attributes(0), CRC(0),
		LastWriteTime(0), CreationTime(0), IsXORed(false)
	{}
};

#define SCRIPT_FILE "irsetup.dat"

class SetupFactoryFile
{
protected:
	CFileStream* m_pInFile;
	std::vector<SFFileEntry> m_vFiles;
	UINT m_nFilenameCodepage;
	int m_nVersion;
	CMemoryStream* m_pScriptData;
	EntryCompression m_eBaseCompression;
	int64_t m_nStartOffset;

	void Init();
	
	bool ReadString(AStream* stream, char* buf);
	bool SkipString(AStream* stream);

	std::string JoinLocalPath(const char* dirName, const char* fileName);

public:
	virtual ~SetupFactoryFile() {}

	virtual bool Open(CFileStream* inFile) = 0;
	virtual int EnumFiles() = 0;

	void Close();
	bool ExtractFile(int index, AStream* outStream);

	size_t GetCount() { return m_vFiles.size(); };
	SFFileEntry GetFile(int index) { return m_vFiles[index]; }

	void SetFileNameEncoding(UINT codePage) { m_nFilenameCodepage = codePage; }
	UINT GetFileNameEncoding() { return m_nFilenameCodepage; }
	int GetVersion() { return m_nVersion; }
	EntryCompression GetCompression() { return m_eBaseCompression; }
};

SetupFactoryFile* OpenInstaller(const wchar_t* filePath);
void FreeInstaller(SetupFactoryFile* file);

#endif // SfFile_h__
