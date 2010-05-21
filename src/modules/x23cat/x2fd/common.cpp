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

const char* GetFileName(const char* path)
{
	const char* fileName = strrchr(path, '\\');
	if (fileName)
		return ++fileName;
	else
		return path;
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
	wchar_t* ext = wcsrchr(path, '.');
	return ext;
}
