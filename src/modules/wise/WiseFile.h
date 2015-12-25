#ifndef WiseFile_h__
#define WiseFile_h__

#include "BaseWiseFile.h"

class CWiseFile : public CBaseWiseFile
{
private:
	HANDLE m_hSourceFile;
	int m_nFilesStartPos;
	std::vector<WiseFileRec> m_vFileList;
	bool m_fIsPkZip;
	
	char* m_pScriptBuf;
	size_t m_nScriptBufSize;
	size_t m_nScriptOffsetBaseFileIndex;
	int m_nFileTypeOffset;

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

	const wchar_t* GetFormatName() { return L"Wise Installer"; }
	const wchar_t* GetCompression() { return L"zlib"; }
};

#endif // WiseFile_h__