#include "StdAfx.h"

void ParseCATPath(const char *pszName, char **ppszCATName, char **ppszFile)
{
	const char *pos=strstr(pszName, "::"), *end=(char*)(pszName + strlen(pszName));
	if(pos==0){
		pos=((char*)pszName) - 2;
		*ppszFile=new char[end - pos - 1];
		*ppszCATName=0;
	}
	else{
		*ppszCATName=new char[pos-pszName + 1];
		memcpy(*ppszCATName, pszName, pos-pszName);
		(*ppszCATName)[pos-pszName]=0;
		*ppszFile=new char[end-pos-2 + 1];
	}
	strcpy(*ppszFile, pos + 2);
}
//---------------------------------------------------------------------------------

bool FileExists(const wchar_t* path)
{
	WIN32_FIND_DATAW fdata;

	HANDLE sr = FindFirstFileW(path, &fdata);
	if (sr != INVALID_HANDLE_VALUE)
	{
		FindClose(sr);
		return true;
	}

	return false;
}

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000i64;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

const char* GetFileName(const char* path)
{
	const char* fileName = strrchr(path, '\\');
	if (fileName)
		return ++fileName;
	else
		return path;
}
