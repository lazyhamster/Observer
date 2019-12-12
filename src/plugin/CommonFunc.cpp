#include "StdAfx.h"
#include "CommonFunc.h"

#include <ShlObj.h>

bool CheckEsc()
{
	DWORD dwNumEvents;
	_INPUT_RECORD inRec;

	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
	if (GetNumberOfConsoleInputEvents(hConsole, &dwNumEvents))
		while (PeekConsoleInputA(hConsole, &inRec, 1, &dwNumEvents) && (dwNumEvents > 0))
		{
			ReadConsoleInputA(hConsole, &inRec, 1, &dwNumEvents);
			if ((inRec.EventType == KEY_EVENT) && (inRec.Event.KeyEvent.bKeyDown) && (inRec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
				return true;
		} //while

		return false;
}

bool FileExists(const std::wstring& path, LPWIN32_FIND_DATAW file_data)
{
	WIN32_FIND_DATAW fdata;

	HANDLE sr = FindFirstFileW(path.c_str(), &fdata);
	if (sr != INVALID_HANDLE_VALUE)
	{
		if (file_data)
			*file_data = fdata;

		FindClose(sr);
		return true;
	}

	return false;
}

bool DirectoryExists(const wchar_t* path)
{
	WIN32_FIND_DATAW fdata;
	bool fResult = false;

	HANDLE sr = FindFirstFileW(path, &fdata);
	if (sr != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0)
			{
				fResult = true;
				break;
			}
		} while (FindNextFileW(sr, &fdata));

		FindClose(sr);
	}

	return fResult;
}

bool IsDiskRoot(const wchar_t* path)
{
	size_t nPathLen = wcslen(path);
	
	return ((nPathLen == 3) && (path[1] == ':') && (path[2] == '\\'))
		|| ((nPathLen == 2) && (path[1] == ':'));
}

bool IsEnoughSpaceInPath(const wchar_t* path, __int64 requiredSize)
{
	if (requiredSize <= 0) return true;
	if (!path) return false;
	
	ULARGE_INTEGER liFreeBytes;
	if (GetDiskFreeSpaceEx(path, &liFreeBytes, NULL, NULL))
		return (liFreeBytes.QuadPart > (ULONGLONG) requiredSize);
	
	return true;
}

bool ForceDirectoryExist(const std::wstring& path)
{
	int nDirResult = !IsDiskRoot(path.c_str()) ? SHCreateDirectory(0, path.c_str()) : ERROR_SUCCESS;
	return (nDirResult == ERROR_SUCCESS) || (nDirResult == ERROR_ALREADY_EXISTS);
}

const wchar_t* ExtractFileName(const wchar_t* fullPath)
{
	const wchar_t* lastSlash = wcsrchr(fullPath, '\\');
	if (lastSlash)
		return lastSlash + 1;
	else
		return fullPath;
}

std::wstring GetDirectoryName(const std::wstring &fullPath, bool includeTrailingDelim)
{
	std::wstring strResult;

	size_t nLastSlash = fullPath.find_last_of('\\');
	if (nLastSlash != std::wstring::npos)
	{
		size_t nCount = includeTrailingDelim ? nLastSlash + 1 : nLastSlash;
		strResult = fullPath.substr(0, nCount);
	}

	return strResult;
}

void IncludeTrailingPathDelim(wchar_t *pathBuf, size_t bufMaxSize)
{
	size_t nPathLen = wcslen(pathBuf);
	if (nPathLen > 0 && pathBuf[nPathLen - 1] != '\\')
		wcscat_s(pathBuf, bufMaxSize, L"\\");
}

void IncludeTrailingPathDelim(std::wstring& pathBuf)
{
	if (pathBuf.length() == 0) return;

	if (pathBuf.at(pathBuf.length() - 1) != L'\\')
		pathBuf.append(L"\\");
}

void CutFileNameFromPath(wchar_t* fullPath, bool includeTrailingDelim)
{
	wchar_t* lastSlashPos = wcsrchr(fullPath, '\\');
	if (lastSlashPos)
	{
		// Always keep trailing delimiter if file is in root folder
		if (includeTrailingDelim || (*(lastSlashPos-1) == ':'))
			*(lastSlashPos + 1) = 0;
		else
			*lastSlashPos = 0;
	}
	else
	{
		*fullPath = 0;
	}
}

void InsertCommas(wchar_t *Dest)
{
	int I;
	for (I=(int)wcslen(Dest)-4;I>=0;I-=3)
		if (Dest[I])
		{
			wmemmove(Dest+I+2,Dest+I+1,wcslen(Dest+I));
			Dest[I+1]=',';
		}
}

std::wstring FormatString(const wchar_t* fmt_str, ...)
{
	const size_t MaxBufferSize = 1024;
	
	wchar_t formatted[MaxBufferSize];
	va_list ap;

	wcscpy_s(&formatted[0], MaxBufferSize, fmt_str);
	va_start(ap, fmt_str);
	_vsnwprintf_s(&formatted[0], MaxBufferSize, MaxBufferSize, fmt_str, ap);
	va_end(ap);

	return std::wstring(formatted);
}

std::wstring FileTimeToString( FILETIME ft )
{
	if (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0)
		return L"-";
	
	SYSTEMTIME stime = {0};
	FileTimeToSystemTime(&ft, &stime);

	return FormatString(L"%02d.%02d.%04d %02d:%02d:%02d", stime.wDay, stime.wMonth, stime.wYear, stime.wHour, stime.wMinute, stime.wSecond);
}

bool IsTimeSet(const FILETIME* ft)
{
	return ft && (ft->dwHighDateTime != 0 || ft->dwLowDateTime != 0);
}

void UpdateFileTime(const wchar_t* path, const FILETIME* createTime, const FILETIME* modTime)
{
	const FILETIME* pCreationTime = IsTimeSet(createTime) ? createTime : NULL;
	const FILETIME* pModificationTime = IsTimeSet(modTime) ? modTime : NULL;

	if (pCreationTime || pModificationTime)
	{
		HANDLE hTmp = CreateFile(path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hTmp != INVALID_HANDLE_VALUE)
		{
			SetFileTime(hTmp, pCreationTime, NULL, pModificationTime);
			CloseHandle(hTmp);
		}
	}
}

static inline bool CheckSingleState(bool isNeeded, DWORD dwState, DWORD dwControlFlags)
{
	return isNeeded == ((dwState & dwControlFlags) != 0);
}

bool CheckControlKeys(const KEY_EVENT_RECORD &evtRec, bool needCtrl, bool needAlt, bool needShift)
{
	return CheckSingleState(needCtrl, evtRec.dwControlKeyState, LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)
		&& CheckSingleState(needAlt, evtRec.dwControlKeyState, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)
		&& CheckSingleState(needShift, evtRec.dwControlKeyState, SHIFT_PRESSED);
}
