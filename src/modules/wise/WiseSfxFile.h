#ifndef WiseSfxFile_h__
#define WiseSfxFile_h__

#include "BaseWiseFile.h"
#include "modulecrt/Streams.h"

class CWiseSfxFile : public CBaseWiseFile
{
private:
	CFileStream* m_pSource;
	int64_t m_nStartOffset;
	int64_t m_nFileSize;
	DWORD m_nCrc32;

	int64_t SearchForMSI(AStream* stream);

public:
	CWiseSfxFile();
	~CWiseSfxFile();

	bool Open(const wchar_t* filePath);

	int NumFiles() { return 1; }
	bool GetFileInfo(int index, WiseFileRec* infoBuf, bool &noMoreItems);

	bool ExtractFile(int index, const wchar_t* destPath);

	const wchar_t* GetFormatName() { return L"Wise SFX Installer"; }
	const wchar_t* GetCompression() { return L"None"; }
};

#endif // WiseSfxFile_h__
