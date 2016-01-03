#include "stdafx.h"
#include "PdfInfo.h"
#include "poppler/DateInfo.h"
#include "poppler/UTF.h"
#include "poppler/UTF8.h"
#include "utils/JSInfo.h"

#define ENDL "\r\n"

static bool TryParseDateTime(GooString* strDate, SYSTEMTIME *sTime)
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
		memset(sTime, 0, sizeof(SYSTEMTIME));
		sTime->wYear = year;
		sTime->wMonth = month;
		sTime->wDay = day;
		sTime->wHour = hour;
		sTime->wMinute = minute;
		sTime->wSecond = sec;

		return true;
	}

	return false;
}

bool TryParseDateTime( GooString* strDate, FILETIME *fTime )
{
	SYSTEMTIME stime = {0};
	return TryParseDateTime(strDate, &stime)
		&& SystemTimeToFileTime(&stime, fTime);
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
		GooString* gstr = obj.getString();
		Unicode *u;
		char buf[8];
		
		output << header;
		int len = TextStringToUCS4(gstr, &u);
		for (int i = 0; i < len; i++)
		{
			int n = mapUTF8(u[i], buf, sizeof(buf));
			buf[n] = 0; // Buffer will be not 0-terminated from prev function
			output << buf;
		}
		output  << ENDL;
	}
	obj.free();
}

static void PrintDateEntry(Dict* dict, const char* key, const char* header, std::stringstream& output)
{
	Object obj;
	if (dict->lookup(key, &obj) && obj.isString())
	{
		GooString* gstr = obj.getString();
		SYSTEMTIME stime;
		
		output << header;
		if (TryParseDateTime(gstr, &stime))
		{
			GooString* timeStr = GooString::format("{0:-0d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d}", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond);
			output << timeStr->getCString();
			delete timeStr;
		}
		else
		{
			output << gstr->getCString();
		}
		output << ENDL;
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
		PrintDateEntry(infoDict, "CreationDate", "CreationDate:   ", sstr);
		PrintDateEntry(infoDict, "ModDate",      "ModDate:        ", sstr);
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
	
	if (doc->getNumPages() > 0)
	{
		sstr << "Page Size:      " << doc->getPageMediaWidth(1) << " x " << doc->getPageMediaHeight(1) << ENDL;
		sstr << "Page Rotate:    " << doc->getPageRotate(1) << ENDL;
	}
	
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
		sprintf_s(tmpBuf, sizeof(tmpBuf), "yes (print:%s copy:%s change:%s addNotes:%s algorithm:%s)",
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
