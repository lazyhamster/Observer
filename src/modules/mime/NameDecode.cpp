#include "StdAfx.h"

static bool SameText(const char* str1, const char* str2)
{
	return _stricmp(str1, str2) == 0;
}

static void GenerateFileName(GMimeObject* entity, wchar_t* dest, size_t destSize)
{
	GMimeContentType* ctype = g_mime_object_get_content_type(entity);

	const char* szType = g_mime_content_type_get_media_type(ctype);
	const char* szSubType = g_mime_content_type_get_media_subtype(ctype);

	std::string strName = "file.dat";
	if (SameText(szType, "image"))
	{
		strName = "image.";
		strName.append(szSubType);
	}
	else if (SameText(szType, "text"))
	{
		if (SameText(szSubType, "html"))
			strName = "index.html";
		else if (SameText(szSubType, "richtext"))
			strName = "message.rtf";
		else
			strName = "message.txt";
	}
	else if (SameText(szType, "application"))
	{
		if (SameText(szSubType, "postscript"))
			strName = "document.ps";
	}
	else if (SameText(szType, "message"))
	{
		if (SameText(szSubType, "rfc822"))
			strName = "message.eml";
	}

	MultiByteToWideChar(CP_UTF8, 0, strName.c_str(), -1, dest, (int)destSize);
}

static void DecodeStr(const char* src, wchar_t* dest, size_t destSize, GMimeParserOptions* parserOpts)
{
	wmemset(dest, 0, destSize);
	if (strstr(src, "=?") != NULL)
	{
		char* szPtr = g_mime_utils_header_decode_text(parserOpts, src);
		MultiByteToWideChar(CP_UTF8, 0, szPtr, -1, dest, (int)destSize);
		g_free(szPtr);
	}
	else
	{
		MultiByteToWideChar(CP_UTF8, 0, src, -1, dest, (int)destSize);
	}
}

static void RemoveUrlComponent(wchar_t* filename)
{
	// Remote query part
	wchar_t* qPos = wcschr(filename, '?');
	if (qPos) *qPos = 0;

	// Remote domain and path
	wchar_t* sPos = wcsrchr(filename, '/');
	if (sPos) wmemmove(filename, sPos + 1, wcslen(sPos + 1) + 1);

	// Remote system path (for file:// URL)
	wchar_t* bPos = wcsrchr(filename, '\\');
	if (bPos) wmemmove(filename, bPos + 1, wcslen(bPos + 1) + 1);
}

void GetEntityName(GMimePart* entity, wchar_t* dest, size_t destSize, GMimeParserOptions* parserOpts)
{
	const char* contentDesp = g_mime_object_get_content_disposition_parameter((GMimeObject*) entity, "filename");
	if (contentDesp)
	{
		DecodeStr(contentDesp, dest, destSize, parserOpts);
		RemoveUrlComponent(dest);
		if (wcslen(dest) > 1) return;
	}

	const char* contentType = g_mime_object_get_content_type_parameter((GMimeObject*) entity, "name");
	if (contentType)
	{
		DecodeStr(contentType, dest, destSize, parserOpts);
		RemoveUrlComponent(dest);
		if (wcslen(dest) > 1) return;
	}

	const char* contetLocation = g_mime_part_get_content_location(entity);
	if (contetLocation)
	{
		DecodeStr(contetLocation, dest, destSize, parserOpts);

		RemoveUrlComponent(dest);
		if (wcslen(dest) > 1) return;
	}

	GenerateFileName((GMimeObject*) entity, dest, destSize);
}
