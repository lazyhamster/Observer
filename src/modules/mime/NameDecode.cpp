#include "StdAfx.h"

#include <sstream>
#include "mimetic/mimetic.h"

using namespace mimetic;
using namespace std;

struct EncodedMIMEFilename
{
	string Charset;
	char Encoding;
	string EncodedFilename;
};

static bool SplitEncodedFilename(string filename, EncodedMIMEFilename &retval)
{
	if (filename.find("=?") != 0)
		return false;

	// Get rid of leading symbols
	string str = filename.substr(2);

	// Find Charset
	size_t pos = str.find('?');
	if (pos == string::npos) return false;

	string strCharset = str.substr(0, pos);
	char cEnc = str[pos + 1];

	if (cEnc != 'Q' && cEnc != 'B')
		return false;

	// Find terminating sequence
	size_t lastPos = str.find("?=");
	if (lastPos == string::npos) return false;
	string encodedName = str.substr(pos + 3, lastPos - pos - 4);

	// Convert charset string to upper case
	transform(strCharset.begin(), strCharset.end(), strCharset.begin(), toupper);

	retval.Encoding = cEnc;
	retval.Charset = strCharset;
	retval.EncodedFilename = encodedName;
	return true;
}

static wstring DecodeFileName(string filename)
{
	string decodedName = filename;
	UINT nCodePage = CP_ACP;
	EncodedMIMEFilename encName;

	if (filename.find("=?") == 0 && SplitEncodedFilename(filename, encName))
	{
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

		// Detect required codepage
		if (encName.Charset.compare("UTF-8") == 0)
		{
			nCodePage = CP_UTF8;
		}
		else if (encName.Charset.compare("KOI8-R") == 0)
		{
			nCodePage = 20866;
		}
		else if (encName.Charset.find("WINDOWS-") == 0)
		{
			string strVal = encName.Charset.substr(8);
			istringstream i(strVal);
			UINT nCP;
			if (i >> nCP)
				nCodePage = nCP;
		}
	}

	wchar_t buf[MAX_PATH] = {0};
	MultiByteToWideChar(nCodePage, 0, decodedName.c_str(), (int) decodedName.length(), buf, 100);
	return buf;
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
		string strLocation = flLocation.value();

		// Erase URL part after ? (parameters)
		size_t nPos = strLocation.find('?');
		if (nPos != string::npos)
			strLocation.erase(nPos);
		// Leave only file name
		nPos = strLocation.find_last_of('/');
		if (nPos != string::npos)
			strLocation.erase(0, nPos + 1);

		strFileName = strLocation;
	}

	// If previous check failed, get name from content type
	if (strFileName.length() == 0)
	{
		ContentType &ctype = head.contentType();
		strFileName = ctype.param("name");

		// If location didn't gave us name, then get generic one
		if (strFileName.length() == 0)
		{
			string strType = ctype.type();
			if (strType.compare("image") == 0)
			{
				strFileName = "image" + ctype.subtype();
			}
			else if (strType.compare("text") == 0)
			{
				if (ctype.subtype().compare("html") == 0)
					strFileName = "index.html";
				else
					strFileName = "message.txt";
			}
			else
			{
				strFileName = "file.bin";
			}
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
