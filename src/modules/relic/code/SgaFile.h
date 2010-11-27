#ifndef SgaFile_h__
#define SgaFile_h__

#include "HWAbstractFile.h"

class CSgaFile : public CHWAbstractStorage
{
protected:
	virtual bool Open(HANDLE inFile);

	virtual void OnGetFileInfo(int index);

public:
	CSgaFile();
	virtual ~CSgaFile();
	
	virtual const wchar_t* GetFormatName() const { return L"Relic SGA"; }
	virtual const wchar_t* GetCompression() const { return L"ZLib/None"; }

	virtual bool ExtractFile(int index, HANDLE outfile);
};

#endif // SgaFile_h__