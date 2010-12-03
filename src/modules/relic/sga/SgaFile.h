#ifndef SgaFile_h__
#define SgaFile_h__

#include "..\base\HWAbstractFile.h"
#include "SgaStructs.h"

class CSgaFile : public CHWAbstractStorage
{
private:
	sga_file_header_t m_oFileHeader;
	sga_data_header_t m_oDataHeader;

	bool ExtractPlain(const HWStorageItem &fileInfo, HANDLE outfile, uint32_t crcVal);
	bool ExtractCompressed(const HWStorageItem &fileInfo, HANDLE outfile, uint32_t crcVal);

protected:
	virtual bool Open(CBasicFile* inFile);

	virtual void OnGetFileInfo(int index) {}

public:
	CSgaFile();
	virtual ~CSgaFile();
	
	virtual const wchar_t* GetFormatName() const { return L"Relic SGA"; }
	virtual const wchar_t* GetCompression() const { return L"ZLib/None"; }
	virtual const wchar_t* GetLabel() const { return m_oFileHeader.sArchiveName; }

	virtual bool ExtractFile(int index, HANDLE outfile);
};

#endif // SgaFile_h__