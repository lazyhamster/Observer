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
	std::string m_sPattern;
	uint32_t m_nBlockSize;
	uint32_t m_nSolidSize;
	uint32_t m_nLastSolid;
	std::vector<int64_t> m_vVolOffs;
	std::vector<int64_t> m_vVolSizes;
	BYTE* m_pOutBuffer;
	size_t m_nOutBufferDataSize;

	std::string ReadGeaString(AStream* data);

public:
	GeaFile();
	~GeaFile();

	virtual bool Open(AStream* inStream);
	virtual void Close();

	virtual int GetFilesCount() { return (int) m_vFiles.size(); }
	virtual bool GetFileDesc(int index, StorageItemInfo* desc);

	virtual bool ExtractFile(int index, AStream* dest);

	virtual void GetFileTypeName(wchar_t* buf, size_t bufSize);
	virtual void GetCompressionName(wchar_t* buf, size_t bufSize);
};

#endif // GeaFile_h__
