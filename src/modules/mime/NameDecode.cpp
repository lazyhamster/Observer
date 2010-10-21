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
static bool FindEncodedWord(const string& input, EncodedWordEntry &word, int &start, int &len)
{
	size_t start_pos = input.find("=?");
	if (start_pos == string::npos) return false;

	size_t sep1_pos = input.find('?', start_pos + 2);
	if ((sep1_pos == string::npos) || (sep1_pos - start_pos < 2)) return false;

	size_t sep2_pos = input.find('?', sep1_pos + 1);
	if ((sep2_pos == string::npos) || (sep2_pos - sep1_pos != 2)) return false;

	size_t end_pos = input.find("?=", sep2_pos + 1);
	if (end_pos == string::npos) return false;

	word.Charset = input.substr(start_pos + 2, sep1_pos - start_pos - 2);
	transform(word.Charset.begin(), word.Charset.end(), word.Charset.begin(), toupper);
	word.Encoding = toupper(input.at(sep1_pos + 1));
	word.EncodedFilename = input.substr(sep2_pos + 1, end_pos - sep2_pos - 1);

	start = start_pos;
	len = end_pos - start_pos + 2;

	return true;
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
	else if (strCharsetUp.compare("KOI8-U") == 0)
	{
		return 21866;
	}
	else if (strCharsetUp.find("WINDOWS-") == 0)
	{
		string strVal = strCharsetUp.substr(8);
		istringstream i(strVal);
		UINT nCP;
		if (i >> nCP)
			return nCP;
	}
	else if (strCharsetUp.find("ISO-8859-") == 0)
	{
		string strVal = strCharsetUp.substr(9);
		istringstream i(strVal);
		UINT nCP;
		if (i >> nCP)
			return (28590 + nCP);
	}

	return CP_ACP;
}

static wstring DecodeWord(EncodedWordEntry encWord)
{
	string decodedName;
	
	// Decode name
	if (encWord.Encoding == 'Q')
	{
		QP::Decoder dec;
		stringstream sstrm;
		ostream_iterator<char> oit(sstrm);
		decode(encWord.EncodedFilename.begin(), encWord.EncodedFilename.end(), dec, oit);

		decodedName = sstrm.str();
	}
	else //if (encName.Encoding == 'B')
	{
		Base64::Decoder dec;
		stringstream sstrm;
		ostream_iterator<char> oit(sstrm);
		decode(encWord.EncodedFilename.begin(), encWord.EncodedFilename.end(), dec, oit);

		decodedName = sstrm.str();
	}

	int nCodePage = Charset2Codepage(encWord.Charset);

	wchar_t buf[MAX_PATH] = {0};
	MultiByteToWideChar(nCodePage, 0, decodedName.c_str(), (int) decodedName.length(), buf, sizeof(buf) / sizeof(buf[0]));

	return buf;
}

static wstring DecodeFileName(const string& filename)
{
	EncodedWordEntry eword;
	int start, len;

	if (FindEncodedWord(filename, eword, start, len))
	{
		wstring strOut = DecodeFileName(filename.substr(0, start))
			+ DecodeWord(eword)
			+ DecodeFileName(filename.substr(start + len));

		return strOut;
	}
	else
	{
		wchar_t buf[MAX_PATH] = {0};
		MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), (int) filename.length(), buf, MAX_PATH);
		return buf;
	}
}

static wstring GenerateFileName(const ContentType &ctype)
{
	string strType = ctype.type();
	string strSybType = ctype.subtype();

	transform(strType.begin(), strType.end(), strType.begin(), tolower);
	transform(strSybType.begin(), strSybType.end(), strSybType.begin(), tolower);

	wstring strResult = L"file.bin";
	if (strType.compare("image") == 0)
	{
		string str = "image." + strSybType;

		strResult.resize(str.length(), ' ');
		copy(str.begin(), str.end(), strResult.begin());
	}
	else if (strType.compare("text") == 0)
	{
		if (strSybType.compare("html") == 0)
			strResult = L"index.html";
		else if (strSybType.compare("richtext") == 0)
			strResult = L"message.rtf";
		else
			strResult = L"message.txt";
	}
	else if (strType.compare("application") == 0)
	{
		if (strSybType.compare("postscript") == 0)
			strResult = L"file.ps";
	}
	else if (strType.compare("message") == 0)
	{
		if (strSybType.compare("rfc822") == 0)
			strResult = L"message.eml";
	}
	
	return strResult;
}

static string FormatChunkName(const char* baseName, int chunkNum)
{
	stringstream sstr;
	sstr << baseName << "*" << chunkNum << "*";
	return sstr.str();
}

static wstring UrlDecodeFileName(const string& input)
{
	enum DecodeState_e
	{
		STATE_SEARCH = 0, ///< searching for an ampersand to convert
		STATE_CONVERTING, ///< convert the two proceeding characters from hex
	};

	DecodeState_e state = STATE_SEARCH;
	string strOut;
	for(unsigned int i = 0; i < input.length(); ++i)
	{
		switch(state)
		{
			case STATE_SEARCH:
			{
				if ((input[i] == '%') && (input.length() - i > 2))
					state = STATE_CONVERTING;
				else
					strOut += input[i];
			}
			break;

			case STATE_CONVERTING:
			{
				// Conversion complete (i.e. don't convert again next iter)
				state = STATE_SEARCH;

				// Create a buffer to hold the hex. For example, if %20, this
				// buffer would hold 20 (in ASCII)
				char pszTempNumBuf[3] = {0};
				strncpy_s(pszTempNumBuf, 3, input.c_str() + i, 2);

				// Ensure both characters are hexadecimal
				bool bBothDigits = true;

				for(int j = 0; j < 2; ++j)
				{
					if(!isxdigit(pszTempNumBuf[j]))
						bBothDigits = false;
				}

				if(!bBothDigits)
					break;

				// Convert two hexadecimal characters into one character
				int nAsciiCharacter;
				sscanf_s(pszTempNumBuf, "%x", &nAsciiCharacter);

				// Concatenate this character onto the output
				strOut += (char*)&nAsciiCharacter;

				// Skip the next character
				i++;
			}
			break;
		}
	}
	
	return DecodeFileName(strOut);
}

static string ConcatParam(const char* baseName, const FieldParamList& params)
{
	string strResult;
	
	int nChunk = 0;
	string strChunkName = FormatChunkName(baseName, nChunk);
	
	FieldParamList::const_iterator bit = params.begin(), eit = params.end();
	for(; bit != eit; ++bit)
	{
		if(bit->name() == strChunkName)
		{
			strResult += bit->value();

			if (nChunk == 0) // Strip encoding and language, first chunk only
			{
				size_t pos = strResult.find('\'');
				if (pos != string::npos)
				{
					pos = strResult.find('\'', pos + 1);
					if (pos != string::npos)
						strResult.erase(0, pos + 1);
				}
			}

			nChunk++;
			strChunkName = FormatChunkName(baseName, nChunk);
		}
	}

	return strResult;
}

static wstring GetNameFromContentDisposition(const Header &head)
{
	const ContentDisposition &cd = head.contentDisposition();
	
	const string &fname = cd.param("filename");
	if (fname.length() > 0)
	{
		return DecodeFileName(fname);
	}

	string strResult = ConcatParam("filename", cd.paramList());
	return UrlDecodeFileName(strResult);
}

static wstring GetNameFromContentType(const Header &head)
{
	const ContentType &ct = head.contentType();

	const string &fname = ct.param("name");
	if (fname.length() > 0)
		return DecodeFileName(fname);

	string strResult = ConcatParam("name", ct.paramList());
	return UrlDecodeFileName(strResult);
}

wstring GetEntityName(MimeEntity* entity)
{
	const Header& head = entity->header();

	wstring wstr = GetNameFromContentDisposition(head);
	if (wstr.length() > 0)
		return wstr;

	wstr = GetNameFromContentType(head);
	if (wstr.length() > 0)
		return wstr;


	// If previous check failed, extract name from Content-Location field
	const Field& flLocation = head.field("Content-Location");
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

	// All checks failed, generated generic name
	const ContentType &ctype = head.contentType();
	return GenerateFileName(ctype);
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
