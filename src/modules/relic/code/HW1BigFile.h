#ifndef HW1BigFile_h__
#define HW1BigFile_h__

#include "HWAbstractFile.h"

class CHW1BigFile : public CHWAbstractStorage
{
private:
protected:
	virtual bool Open(HANDLE inFile);
	virtual void Close();

public:
	CHW1BigFile();
	virtual ~CHW1BigFile();

	virtual const wchar_t* GetFormatName() const { return L"HomeWorld 1 BIG"; }
	virtual const wchar_t* GetCompression() const { return L"ZLib/None"; }

	virtual bool ExtractFile(int index, HANDLE outfile);
};

#endif // HW1BigFile_h__