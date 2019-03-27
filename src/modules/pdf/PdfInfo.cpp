#include "stdafx.h"
#include "PdfInfo.h"
#include "poppler/DateInfo.h"
#include "poppler/UTF.h"
#include "poppler/UnicodeMapFuncs.h"
#include "utils/JSInfo.h"

#define ENDL "\r\n"

static bool TryParseDateTime(const GooString* strDate, SYSTEMTIME *sTime)
{
	if (strDate == nullptr)
		return false;

	int year, month, day;
	int hour, minute, sec;
	int tzhour, tzmin;
	char tz;

	//TODO: deal with timezone
	if (parseDateString(strDate->c_str(), &year, &month, &day, &hour, &minute, &sec, &tz, &tzhour, &tzmin))
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

static bool TryParseDateTime(const GooString* strDate, FILETIME *fTime)
{
	SYSTEMTIME stime = {0};
	return TryParseDateTime(strDate, &stime)
		&& SystemTimeToFileTime(&stime, fTime);
}

//////////////////////////////////////////////////////////////////////////

bool PdfInfo::LoadInfo( PDFDoc* doc, bool openAllFiles )
{
	Cleanup();
	
	Catalog* cat = doc->getCatalog();
	if (!cat) return false;
	
	// Get embedded files
	for (int i = 0; i < cat->numEmbeddedFiles(); i++)
	{
		FileSpec* file = cat->embeddedFile(i);
		if (file->isOk())
			m_vFiles.push_back(new PdfEmbeddedFile(file));
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
			{
				FileSpec* spec = new FileSpec(static_cast<AnnotFileAttachment *>(ant)->getFile());
				m_vFiles.push_back(new PdfEmbeddedFile(spec));
			}
		}
	}

	char scriptNameBuf[MAX_PATH];
	for (int i = 0; i < cat->numJS(); i++)
	{
		GooString* jsName = cat->getJSName(i);
		GooString* jsText = cat->getJS(i);
		if (jsText)
		{
			std::string scriptName = jsName ? jsName->toStr() : "";
			if (scriptName.empty())
			{
				sprintf_s(scriptNameBuf, MAX_PATH, "script%04d", i);
				scriptName = scriptNameBuf;
			}

			m_vFiles.push_back(new PdfScriptFile(scriptName.c_str(), jsText));
		}
	}

	// Generate info
	if (openAllFiles || m_vFiles.size() > 0)
	{
		std::string metadata;
		LoadMetaData(doc, metadata);
		m_vFiles.insert(m_vFiles.begin(), new PdfMetadataFile(metadata));
	}

	if (m_vFiles.size() > 0)
	{
		docRef = doc;
		return true;
	}

	return false;
}

void PdfInfo::Cleanup()
{
	std::for_each(m_vFiles.begin(), m_vFiles.end(), [](PdfPseudoFile* f) { delete f; });
	m_vFiles.clear();
	
	if (docRef)
	{
		delete docRef;
		docRef = nullptr;
	}
}

static void PrintEntry(Dict* dict, const char* key, const char* header, std::stringstream& output)
{
	Object obj = dict->lookup(key);
	if (obj.isString())
	{
		const GooString* gstr = obj.getString();
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
}

static void PrintDateEntry(Dict* dict, const char* key, const char* header, std::stringstream& output)
{
	Object obj = dict->lookup(key);
	if (obj.isString())
	{
		const GooString* gstr = obj.getString();
		SYSTEMTIME stime;
		
		output << header;
		if (TryParseDateTime(gstr, &stime))
		{
			GooString* timeStr = GooString::format("{0:-0d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d}", stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond);
			output << timeStr->c_str();
			delete timeStr;
		}
		else
		{
			output << gstr->toStr();
		}
		output << ENDL;
	}
}

void PdfInfo::LoadMetaData( PDFDoc *doc, std::string &output )
{
	std::stringstream sstr;
	
	Object info = doc->getDocInfo();
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
		unsigned char* fileKey;
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
			doc->okToPrint(true) ? "yes" : "no",
			doc->okToCopy(true) ? "yes" : "no",
			doc->okToChange(true) ? "yes" : "no",
			doc->okToAddNotes(true) ? "yes" : "no",
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
	const GooString* strMeta = doc->readMetadata();
	if (strMeta)
	{
		sstr << ENDL << "Metadata:" << ENDL << strMeta->toStr() << ENDL;
		delete strMeta;
	}

	output = sstr.str();
}

//////////////////////////////////////////////////////////////////////////

static std::wstring ConvertNameToUnicode(const char* name, int nameLen)
{
	wchar_t tmp[MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, name, nameLen, tmp, MAX_PATH);
	return tmp;
}

static PseudoFileSaveResult DumpStringToFile(const wchar_t* destPath, const std::string &content)
{
	DWORD dwBytes;
	HANDLE hf = CreateFileW(destPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf == INVALID_HANDLE_VALUE) return PseudoFileSaveResult::PFSR_ERROR_WRITE;

	PseudoFileSaveResult rval = PseudoFileSaveResult::PFSR_OK;
	if (!WriteFile(hf, content.c_str(), (DWORD)content.size(), &dwBytes, NULL))
		rval = PseudoFileSaveResult::PFSR_ERROR_WRITE;

	CloseHandle(hf);
	return rval;
}

PdfMetadataFile::PdfMetadataFile(const std::string& data)
{
	m_strName = L"{pdf_info}.txt";
	m_strMetadata = data;
}

PseudoFileSaveResult PdfMetadataFile::Save(const wchar_t* filePath) const
{
	return DumpStringToFile(filePath, m_strMetadata);
}

PdfScriptFile::PdfScriptFile(const char* name, const GooString* text)
{
	m_strName = ConvertNameToUnicode(name, -1);
	m_strText = text->toStr();
}

PseudoFileSaveResult PdfScriptFile::Save(const wchar_t* filePath) const
{
	return DumpStringToFile(filePath, m_strText);
}

PdfEmbeddedFile::PdfEmbeddedFile(FileSpec* spec)
{
	m_pSpec = spec;
	m_pEmbFile = spec->getEmbeddedFile();

	const GooString* strName = m_pSpec->getFileName();
	if (strName->hasUnicodeMarker())
	{
		wchar_t tmpBuf[MAX_PATH] = { 0 };
		
		int numWChars = (strName->getLength() - 2) / 2;
		size_t nextCharPos = 0;
		const char* pChar = strName->c_str() + 2;
		for (int i = 0; i < numWChars; i++)
		{
			tmpBuf[nextCharPos] = (pChar[0] << 8) | pChar[1];
			nextCharPos++;
			pChar += 2;
		}
		tmpBuf[nextCharPos] = 0;

		m_strName = tmpBuf;
	}
	else
	{
		m_strName = ConvertNameToUnicode(strName->c_str(), strName->getLength());
	}
}

PdfEmbeddedFile::~PdfEmbeddedFile()
{
	delete m_pEmbFile;
	delete m_pSpec;
}

void PdfEmbeddedFile::GetCreationTime(FILETIME* time) const
{
	TryParseDateTime(m_pEmbFile->createDate(), time);
}

void PdfEmbeddedFile::GetModificationTime(FILETIME* time) const
{
	TryParseDateTime(m_pEmbFile->modDate(), time);
}

PseudoFileSaveResult PdfEmbeddedFile::Save(const wchar_t* filePath) const
{
	if (!m_pEmbFile->isOk())
		return PseudoFileSaveResult::PFSR_ERROR_READ;

	FILE* outF = _wfopen(filePath, L"wb");
	if (!outF) return PseudoFileSaveResult::PFSR_ERROR_WRITE;

	bool ret = m_pEmbFile->save2(outF);
	return ret ? PseudoFileSaveResult::PFSR_OK : PseudoFileSaveResult::PFSR_ERROR_WRITE;
}
