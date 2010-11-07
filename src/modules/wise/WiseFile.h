#ifndef WiseFile_h__
#define WiseFile_h__

struct WiseFileRec
{
	wchar_t FileName[MAX_PATH];
	int StartOffset;
	int PackedSize;
	int UnpackedSize;
	unsigned long CRC32;
};

class CWiseFile
{
private:
	HANDLE m_hSourceFile;
	int m_nFilesStartPos;

	bool Approximate(int &approxOffset, bool &isPkZip);
	bool FindReal(int approxOffset, bool isPkZip, int &realOffset);

public:
	CWiseFile();
	~CWiseFile();

	bool Open(const wchar_t* filePath);
	
	int NumFiles() { return 0; }
	bool GetFileInfo(int index, WiseFileRec* infoBuf, bool &noMoreItems);
};

#endif // WiseFile_h__