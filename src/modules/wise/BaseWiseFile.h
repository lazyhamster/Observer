#ifndef BaseWiseFile_h__
#define BaseWiseFile_h__

struct WiseFileRec
{
	wchar_t FileName[MAX_PATH];
	int64_t StartOffset;
	int64_t EndOffset;
	int64_t PackedSize;
	int64_t UnpackedSize;
	unsigned long CRC32;
};

class CBaseWiseFile
{
public:
	virtual ~CBaseWiseFile() {}

	virtual bool Open(const wchar_t* filePath) = 0;
	virtual int NumFiles() = 0;
	virtual bool GetFileInfo(int index, WiseFileRec* infoBuf, bool &noMoreItems) = 0;
	virtual bool ExtractFile(int index, const wchar_t* destPath) = 0;
	virtual const wchar_t* GetFormatName() = 0;
	virtual const wchar_t* GetCompression() = 0;
};

#endif // BaseWiseFile_h__
