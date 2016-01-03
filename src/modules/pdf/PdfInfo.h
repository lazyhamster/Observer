#ifndef PdfInfo_h__
#define PdfInfo_h__

#include "poppler/PDFDoc.h"
#include "poppler/FileSpec.h"

struct PdfScriptData
{
	std::string Name;
	std::string Text;
};

struct PdfInfo
{
	PDFDoc* docRef;
	
	std::string metadata;
	std::vector<FileSpec*> embFiles;
	std::vector<PdfScriptData> scripts;

	PdfInfo() : docRef(nullptr) {}

	bool LoadInfo(PDFDoc* doc);
	void Cleanup();

	bool HasFiles()
	{
		return (embFiles.size() + scripts.size()) > 0;
	}

private:
	void LoadMetaData(PDFDoc *doc, std::string &output);
};

bool TryParseDateTime(GooString* strDate, FILETIME *fTime);

#endif // PdfInfo_h__
