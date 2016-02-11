#ifndef GeaFile_h__
#define GeaFile_h__

#include "BaseGenteeFile.h"

struct geafiledesc
{
	uint32_t  flags;  // Flags
	FILETIME  ft;     // File time
	uint32_t  size;   // File size
	uint32_t  attrib; // File attribute
	uint32_t  crc;    // File CRC                  
	uint32_t  hiver;  // Hi version
	uint32_t  lowver; // Low version                  
	uint32_t  idgroup;  // id of the group
	uint32_t  idpass;   // id of the password
	uint32_t  compsize; // The compressed size
	int64_t   offset;
	std::string name;     // the name of the file
	std::string subfolder;  // the name of the subfolder

	geafiledesc() : flags(0), size(0), compsize(0), attrib(0), crc(0), hiver(0), lowver(0), idgroup(0), idpass(0), offset(0) {}
};

class GeaFile : public BaseGenteeFile
{
private:
	std::vector<geafiledesc> m_vFiles;
	std::vector<uint32_t> m_vPasswordCRC;
	std::vector<std::string> m_vPasswords;
	std::string m_sPattern;
	uint32_t m_nBlockSize;
	uint32_t m_nSolidSize;
	uint32_t m_nLastSolid;
	std::vector<int64_t> m_vVolOffs;
	std::vector<int64_t> m_vVolSizes;
	BYTE* m_pOutBuffer;
	size_t m_nOutBufferDataSize;
	CMemoryStream m_pMovedData;
	uint32_t m_nHeadSize;
	std::wstring m_sCompressionName;

	int64_t m_nStartOffset;

	std::string ReadGeaString(AStream* data);
	bool ReadCompressedBlock(int64_t offs, uint32_t size, BYTE* buf);
	bool CheckPassword(uint32_t passIndex, const char* password);

public:
	GeaFile();
	virtual ~GeaFile();

	virtual bool Open(AStream* inStream, int64_t startOffset, bool ownStream);
	virtual void Close();

	virtual int GetFilesCount() { return (int) m_vFiles.size(); }
	virtual bool GetFileDesc(int index, StorageItemInfo* desc);

	virtual GenteeExtractResult ExtractFile(int index, AStream* dest, const char* password);

	virtual const wchar_t* GetFileTypeName() { return L"Gentee Archive (GEA)"; }
	virtual const wchar_t* GetCompressionName();
};

#endif // GeaFile_h__
