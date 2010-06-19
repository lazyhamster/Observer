#include "StdAfx.h"

#include <sstream>
#include <vector>
#include "mimetic/mimetic.h"

using namespace mimetic;
using namespace std;

struct EncodedWordEntry
{
	string Charset;
	char Encoding;
	string EncodedFilename;
};

// See RFC2047 for full definition of the encoded-word in MIME
// Format: encoded-word = "=?" charset "?" encoding "?" encoded-text "?="
static bool SplitEncodedFilename(string filename, vector<EncodedWordEntry> &ewordlist)
{
	string str = filename;
	ewordlist.clear();

	while (str.length() > 0)
	{
		// Find board sequences
		size_t startPos = str.find("=?");
		size_t qmPos = (startPos != string::npos) ? str.find('?', startPos + 2) : string::npos;
		size_t endPos = (qmPos != string::npos) ? str.find("?=", qmPos + 3) : string::npos;

		if (startPos != 0 || endPos == string::npos)
		{
			ewordlist.clear();
			return false;
		}

		string eWord = str.substr(startPos + 2, endPos - 2);
		if (eWord.length() > 0)
		{
			qmPos -= 2; // Because be removed leading sequence

			EncodedWordEntry nextEntry;
			nextEntry.Charset = eWord.substr(0, qmPos);
			nextEntry.Encoding = eWord[qmPos + 1];
			nextEntry.EncodedFilename = eWord.substr(qmPos + 3);

			// Check for recognizable encoding
			if ((nextEntry.Encoding == 'Q' || nextEntry.Encoding == 'B') && (nextEntry.EncodedFilename.length() > 0))
				ewordlist.push_back(nextEntry);
		}

		// Delete decoded part
		str.erase(0, endPos + 2);
		// Delete all leading whitespace characters
		while (str[0] == ' ')
			str.erase(0, 1);
	} // while

	return (ewordlist.size() > 0);
}

static UINT Charset2Codepage(string &charset)
{
	// Convert charset string to upper case
	string strCharsetUp(charset);
	transform(charset.begin(), charset.end(), strCharsetUp.begin(), toupper);
	
	if (strCharsetUp.compare("UTF-8") == 0)
	{
		return CP_UTF8;
	}
	else if (strCharsetUp.compare("KOI8-R") == 0)
	{
		return 20866;
	}
	else if (strCharsetUp.find("WINDOWS-") == 0)
	{
		string strVal = strCharsetUp.substr(8);
		istringstream i(strVal);
		UINT nCP;
		if (i >> nCP)
			return nCP;
	}

	return CP_ACP;
}

static wstring DecodeFileName(string filename)
{
	vector<EncodedWordEntry> encList;
	wchar_t buf[MAX_PATH] = {0};

	if (SplitEncodedFilename(filename, encList))
	{
		wstring retval;
		UINT nCodePage = CP_ACP;

		for (size_t i = 0; i < encList.size(); i++)
		{
			EncodedWordEntry &encName = encList[i];
			string decodedName;
			
			// Decode name
			if (encName.Encoding == 'Q')
			{
				QP::Decoder dec;
				stringstream sstrm;
				ostream_iterator<char> oit(sstrm);
				decode(encName.EncodedFilename.begin(), encName.EncodedFilename.end(), dec, oit);

				decodedName = sstrm.str();
			}
			else //if (encName.Encoding == 'B')
			{
				Base64::Decoder dec;
				stringstream sstrm;
				ostream_iterator<char> oit(sstrm);
				decode(encName.EncodedFilename.begin(), encName.EncodedFilename.end(), dec, oit);

				decodedName = sstrm.str();
			}

			nCodePage = Charset2Codepage(encName.Charset);
			memset(buf, 0, sizeof(buf));
			MultiByteToWideChar(nCodePage, 0, decodedName.c_str(), (int) decodedName.length(), buf, sizeof(buf) / sizeof(buf[0]));

			retval += buf;
		}  // for

		return retval;
	}
	else
	{
		MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), (int) filename.length(), buf, sizeof(buf) / sizeof(buf[0]));
		return buf;
	}
}

static string GenerateFileName(ContentType &ctype)
{
	string strType = ctype.type();
	string strSybType = ctype.subtype();

	transform(strType.begin(), strType.end(), strType.begin(), tolower);
	transform(strSybType.begin(), strSybType.end(), strSybType.begin(), tolower);

	string strResult = "file.bin";
	if (strType.compare("image") == 0)
	{
		strResult = "image." + strSybType;
	}
	else if (strType.compare("text") == 0)
	{
		if (strSybType.compare("html") == 0)
			strResult = "index.html";
		else if (strSybType.compare("richtext") == 0)
			strResult = "message.rtf";
		else
			strResult = "message.txt";
	}
	else if (strType.compare("application") == 0)
	{
		if (strSybType.compare("postscript") == 0)
			strResult = "file.ps";
	}
	
	return strResult;
}

wstring GetEntityName(MimeEntity* entity)
{
	Header& head = entity->header();
	string strFileName;	

	// First try to get direct file name
	ContentDisposition &cd = head.contentDisposition();
	strFileName = cd.param("filename");

	// If previous check failed, extract name from Content-Location field
	if (strFileName.length() == 0)
	{
		Field& flLocation = head.field("Content-Location");
		string strLocationVal = flLocation.value();
		if (strLocationVal.length() > 0)
		{
			wstring strLocation = DecodeFileName(strLocationVal);
			if (strLocation.length() > 0)
			{
				// Erase URL part after ? (parameters)
				size_t nPos = strLocation.find('?');
				if (nPos != string::npos)
					strLocation.erase(nPos);
				// Leave only file name
				nPos = strLocation.find_last_of(L"/\\");
				if (nPos != string::npos)
					strLocation.erase(0, nPos + 1);

				if (strLocation.length() > 0)
					return strLocation;
			}
		}
	}

	// If previous check failed, get name from content type
	if (strFileName.length() == 0)
	{
		ContentType &ctype = head.contentType();
		strFileName = ctype.param("name");

		// If location didn't gave us name, then get generic one
		if (strFileName.length() == 0)
		{
			strFileName = GenerateFileName(ctype);
		}
	}

	return DecodeFileName(strFileName);
}

void AppendDigit(wstring &fileName, int num)
{
	wchar_t buf[10] = {0};
	swprintf_s(buf, sizeof(buf) / sizeof(buf[0]), L",%d", num);

	size_t p = fileName.find_last_of('.');
	if (p == string::npos)
		fileName += buf;
	else
		fileName.insert(p, buf);
}
