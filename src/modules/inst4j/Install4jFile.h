#ifndef Install4jFile_h__
#define Install4jFile_h__

#include "modulecrt/Streams.h"

struct DataArchiveInfo
{
	std::string Name;
	uint32_t Size;
	
	int64_t Offset;
	bool IsCompressed;

	DataArchiveInfo() : Offset(0), IsCompressed(false), Size(0) {}
};

class CInstall4jFile
{
public:
	CInstall4jFile();
	~CInstall4jFile();

	bool Open(const wchar_t* path);
	void Close();

	size_t NumArcs() const { return m_vArchList.size(); }
	DataArchiveInfo GetArcInfo(size_t index) { return m_vArchList.at(index); }

	bool ExtractArc(size_t index, const wchar_t* destPath);
private:
	CFileStream* m_pInStream;
	std::vector<DataArchiveInfo> m_vArchList;
	int64_t m_nArchDataStart;

	CInstall4jFile(const CInstall4jFile& other) = delete;
	CInstall4jFile& operator=(const CInstall4jFile& rhs) = delete;

	bool InternalOpen(CFileStream* inStream);
};

#endif // Install4jFile_h__
