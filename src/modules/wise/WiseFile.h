#ifndef WiseFile_h__
#define WiseFile_h__

struct WiseFileRec
{
	wchar_t FileName[MAX_PATH];
	int StartOffset;
	int EndOffset;
	int PackedSize;
	int UnpackedSize;
	unsigned long CRC32;
};

class CWiseFile
{
private:
	HANDLE m_hSourceFile;
	int m_nFilesStartPos;
	std::vector<WiseFileRec> m_vFileList;
	bool m_fIsPkZip;
	
	char* m_pScriptBuf;
	size_t m_nScriptBufSize;
	size_t m_nScriptOffsetBaseFileIndex;

	bool Approximate(int &approxOffset, bool &isPkZip);
	bool FindReal(int approxOffset, bool isPkZip, int &realOffset);

	bool TryResolveFileName(WiseFileRec *infoBuf);

public:
	CWiseFile();
	~CWiseFile();

	bool Open(const wchar_t* filePath);
	
	int NumFiles() { return (int) m_vFileList.size(); }
	bool GetFileInfo(int index, WiseFileRec* infoBuf, bool &noMoreItems);

	bool ExtractFile(int index, const wchar_t* destPath);
};

#endif // WiseFile_h__