#include "stdafx.h"
#include "PdfInfo.h"
#include "poppler/DateInfo.h"
#include "utils/JSInfo.h"

#define ENDL "\r\n"

bool TryParseDateTime( GooString* strDate, FILETIME *fTime )
{
	if (strDate == nullptr)
		return false;
	
	int year, month, day;
	int hour, minute, sec;
	int tzhour, tzmin;
	char tz;
	
	//TODO: deal with timezone
	if (parseDateString(strDate->getCString(), &year, &month, &day, &hour, &minute, &sec, &tz, &tzhour, &tzmin))
	{
		SYSTEMTIME stime = {0};
		stime.wYear = year;
		stime.wMonth = month;
		stime.wDay = day;
		stime.wHour = hour;
		stime.wMinute = minute;
		stime.wSecond = sec;

		return SystemTimeToFileTime(&stime, fTime);
	}

	return false;
}

bool PdfInfo::LoadInfo( PDFDoc* doc )
{
	Catalog* cat = doc->getCatalog();
	
	// Get embedded files
	for (int i = 0; i < cat->numEmbeddedFiles(); i++)
	{
		FileSpec* file = cat->embeddedFile(i);
		if (file->isOk())
			embFiles.push_back(file);
	}

	// Get page attachments
	int numPages = cat->getNumPages();
	for (int i = 0; i < numPages; i++)
	{
		Page* page = cat->getPage(i + 1);
		Annots* annots = page->getAnnots();
		if (!annots) break;

		for (int j = 0; j < annots->getNumAnnots(); j++)
		{
			Annot* ant = annots->getAnnot(j);
			if ((ant->getType() == Annot::typeFileAttachment) && ant->isOk())
				embFiles.push_back(new FileSpec(static_cast<AnnotFileAttachment *>(ant)->getFile()));
		}
	}

	for (int i = 0; i < cat->numJS(); i++)
	{
		GooString* jsName = cat->getJSName(i);
		GooString* jsText = cat->getJS(i);
		if (jsText)
		{
			PdfScriptData scriptData;
			scriptData.Name = jsName && jsName->getLength() ? jsName->getCString() : "";
			scriptData.Text = jsText->getCString();

			scripts.push_back(scriptData);
		}
	}

	// Generate info
	LoadMetaData(doc, metadata);
		
	docRef = doc;
	return true;
}

void PdfInfo::Cleanup()
{
	if (docRef)
		delete docRef;

	metadata.clear();
	
	std::for_each(embFiles.begin(), embFiles.end(), [] (FileSpec* spec) { delete spec; });
	embFiles.clear();

	scripts.clear();
}

static void PrintEntry(Dict* dict, const char* key, const char* header, std::stringstream& output)
{
	Object obj;
	if (dict->lookup(key, &obj) && obj.isString())
	{
		output << header << obj.getString()->getCString() << ENDL;
	}
	obj.free();
}

void PdfInfo::LoadMetaData( PDFDoc *doc, std::string &output )
{
	std::stringstream sstr;
	
	Object info;
	doc->getDocInfo(&info);
	if (info.isDict())
	{
		Dict* infoDict = info.getDict();

		PrintEntry(infoDict, "Title",		 "Title:          ", sstr);
		PrintEntry(infoDict, "Subject",      "Subject:        ", sstr);
		PrintEntry(infoDict, "Keywords",     "Keywords:       ", sstr);
		PrintEntry(infoDict, "Author",       "Author:         ", sstr);
		PrintEntry(infoDict, "Creator",      "Creator:        ", sstr);
		PrintEntry(infoDict, "Producer",     "Producer:       ", sstr);
		PrintEntry(infoDict, "CreationDate", "CreationDate:   ", sstr);
		PrintEntry(infoDict, "ModDate",      "ModDate:        ", sstr);
	}
	info.free();

	Catalog* catalog = doc->getCatalog();

	// print tagging info
	sstr << "Tagged:         " << ((catalog->getMarkInfo() & Catalog::markInfoMarked) ? "yes" : "no") << ENDL;
	sstr << "UserProperties: " << ((catalog->getMarkInfo() & Catalog::markInfoUserProperties) ? "yes" : "no") << ENDL;
	sstr << "Suspects:       " << ((catalog->getMarkInfo() & Catalog::markInfoSuspects) ? "yes" : "no") << ENDL;

	// print form info
	switch (catalog->getFormType())
	{
		case Catalog::NoForm:
			sstr << "Form:           none" << ENDL;
			break;
		case Catalog::AcroForm:
			sstr << "Form:           AcroForm" << ENDL;
			break;
		case Catalog::XfaForm:
			sstr << "Form:           XFA" << ENDL;
			break;
	}

	// print javascript info
	{
		JSInfo jsInfo(doc);
		jsInfo.scanJS(doc->getNumPages());
		sstr << "JavaScript:     " << (jsInfo.containsJS() ? "yes" : "no") << ENDL;
	}

	// print page count
	sstr << "Pages:          " << doc->getNumPages() << ENDL;

	// print encryption info
	sstr << "Encrypted:      ";
	if (doc->isEncrypted())
	{
		Guchar *fileKey;
		CryptAlgorithm encAlgorithm;
		int keyLength;
		doc->getXRef()->getEncryptionParameters(&fileKey, &encAlgorithm, &keyLength);

		const char *encAlgorithmName = "unknown";
		switch (encAlgorithm)
		{
			case cryptRC4:
				encAlgorithmName = "RC4";
				break;
			case cryptAES:
				encAlgorithmName = "AES";
				break;
			case cryptAES256:
				encAlgorithmName = "AES-256";
				break;
		}

		char tmpBuf[512] = {0};
		sprintf_s(tmpBuf, 512, "yes (print:%s copy:%s change:%s addNotes:%s algorithm:%s)",
			doc->okToPrint(gTrue) ? "yes" : "no",
			doc->okToCopy(gTrue) ? "yes" : "no",
			doc->okToChange(gTrue) ? "yes" : "no",
			doc->okToAddNotes(gTrue) ? "yes" : "no",
			encAlgorithmName);
		sstr << tmpBuf << ENDL;
	} else {
		sstr << "no" << ENDL;
	}

	// print linearization info
	sstr << "Optimized:      " << (doc->isLinearized() ? "yes" : "no") << ENDL;

	// print PDF version
	sstr << "PDF version:    " << doc->getPDFMajorVersion() << "." << doc->getPDFMinorVersion() << ENDL;

	// print the metadata
	GooString* strMeta = doc->readMetadata();
	if (strMeta)
	{
		sstr << ENDL << "Metadata:" << ENDL << strMeta->getCString() << ENDL;
		delete strMeta;
	}

	output = sstr.str();
}
