#include "StdAfx.h"

#include "mimetic/mimetic.h"

using namespace mimetic;
using namespace std;

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

	wchar_t buf[MAX_PATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, strFileName.c_str(), (int) strFileName.length(), buf, 100);
	return buf;
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
