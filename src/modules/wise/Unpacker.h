#ifndef Unpacker_h__
#define Unpacker_h__

struct InflatedDataInfo 
{
	unsigned int packedSize;
	unsigned int unpackedSize;
	unsigned long crc;
};

bool inflateData(HANDLE inFile, char* &memBuf, size_t &memBufSize, InflatedDataInfo &info);
bool inflateData(HANDLE inFile, const wchar_t* outFileName, InflatedDataInfo &info);

#endif // Unpacker_h__