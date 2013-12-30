#ifndef PdfInfo_h__
#define PdfInfo_h__

#include "poppler/PDFDoc.h"
#include "poppler/FileSpec.h"

struct PdfInfo
{
	PDFDoc* docRef;
	
	std::string metadata;
	std::vector<FileSpec*> embFiles;

	PdfInfo() : docRef(nullptr) {}

	bool LoadInfo(PDFDoc* doc);
	void LoadMetaData(PDFDoc *doc, std::string &output);
	void Cleanup();
};

bool TryParseDateTime(GooString* strDate, FILETIME *fTime);

#endif // PdfInfo_h__
