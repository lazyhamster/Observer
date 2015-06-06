#ifndef ViseFile_h__
#define ViseFile_h__

#include "modulecrt/Streams.h"

struct ViseFileEntry 
{
	std::string Name;
	uint32_t PackedSize;
	uint32_t UnpackedSize;
	FILETIME LastWriteTime;
	int64_t StartOffset;

	ViseFileEntry()
	{
		Name = "";
		PackedSize = UnpackedSize = 0;
		StartOffset = 0;
		memset(&LastWriteTime, 0, sizeof(LastWriteTime));
	}
};

class CViseFile
{
public:
	CViseFile();
	~CViseFile();

	bool Open(const wchar_t* filePath);
	void Close();

	size_t GetFilesCount() const { return m_vFiles.size(); }
	const ViseFileEntry& GetFileInfo(size_t index);

	bool ExtractFile(size_t fileIndex, AStream* dest);
private:
	std::wstring m_sLocation;
	std::shared_ptr<AStream> m_pInFile;
	int64_t m_nDataStartOffset;
	std::vector<ViseFileEntry> m_vFiles;
	int64_t m_nScriptStartOffset;

	bool ReadServiceFiles(std::shared_ptr<AStream> inStream);
	bool ReadViseData1(std::shared_ptr<AStream> inStream);
	bool ReadViseScript(std::shared_ptr<AStream> inStream);

	bool ReadViseString(std::shared_ptr<AStream> inStream, char* strBuf);
};

#endif // ViseFile_h__
