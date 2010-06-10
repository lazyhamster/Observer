#include "StdAfx.h"

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

const wchar_t* GetFileName(const wchar_t* path)
{
	const wchar_t* fileName = wcsrchr(path, '\\');
	if (fileName)
		return ++fileName;
	else
		return path;
}

wchar_t* GetFileExt(wchar_t* path)
{
	wchar_t *ptr = path + wcslen(path);
	while (ptr > path)
	{
		if (*ptr == '.')
			return ptr;
		else if (*ptr == '\\')
			break;

		ptr--;
	}
	
	return NULL;
}

const wchar_t* GetFileExt(const wchar_t* path)
{
	const wchar_t *ptr = path + wcslen(path);
	while (ptr > path)
	{
		if (*ptr == '.')
			return ptr;
		else if (*ptr == '\\')
			break;

		ptr--;
	}

	return NULL;
}

//---------------------------------------------------------------------------------
wchar_t * ChangeFileExtension(const wchar_t *pszFileName, const wchar_t *pszExt)
{
	const wchar_t *ch, *end=pszFileName + wcslen(pszFileName);
	wchar_t *newfile;

	for(ch=end; ch >= pszFileName && *ch!='.' && *ch!='\\'; ch--)
		;;

	size_t len;
	if(*ch=='\\')
		len=end - pszFileName;
	else if(ch < pszFileName)
		len=end - ch - 1;
	else
		len=ch - pszFileName;
	size_t extlen=wcslen(pszExt);
	newfile=new wchar_t[len + extlen + 2];
	newfile[len + extlen + 1]=0;
	memcpy(newfile, pszFileName, len * sizeof(wchar_t));
	newfile[len]='.';
	memcpy(newfile + len + 1, pszExt, extlen * sizeof(wchar_t));
	return newfile;
}
