#include "StdAfx.h"
#include "CommonFunc.h"

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

bool FileExists(const wchar_t* path, LPWIN32_FIND_DATAW file_data)
{
	WIN32_FIND_DATAW fdata;

	HANDLE sr = FindFirstFileW(path, &fdata);
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

// Returns total number of items added
int CollectFileList(ContentTreeNode* node, ContentNodeList &targetlist, __int64 &totalSize, bool recursive)
{
	int numItems = 0;

	if (node->IsDir())
	{
		if (recursive)
		{
			// Iterate through sub directories
			for (SubNodesMap::const_iterator cit = node->subdirs.begin(); cit != node->subdirs.end(); cit++)
			{
				ContentTreeNode* child = cit->second;
				numItems += CollectFileList(child, targetlist, totalSize, recursive);
			} //for
		}

		// Iterate through files
		for (SubNodesMap::const_iterator cit = node->files.begin(); cit != node->files.end(); cit++)
		{
			ContentTreeNode* child = cit->second;
			targetlist.push_back(child);
			numItems++;
			totalSize += child->GetSize();
		} //for

		// If it is empty directory then just add it
		if (node->GetChildCount() == 0)
			targetlist.push_back(node);
	}
	else
	{
		targetlist.push_back(node);
		numItems++;
		totalSize += node->GetSize();
	}

	return numItems;
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

bool ForceDirectoryExist(const wchar_t* path)
{
	int nDirResult = !IsDiskRoot(path) ? SHCreateDirectory(0, path) : ERROR_SUCCESS;
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

wstring GetDirectoryName(const wstring &fullPath, bool includeTrailingDelim)
{
	wstring strResult;

	size_t nLastSlash = fullPath.find_last_of('\\');
	if (nLastSlash != wstring::npos)
	{
		size_t nCount = includeTrailingDelim ? nLastSlash + 1 : nLastSlash;
		strResult = fullPath.substr(0, nCount);
	}

	return strResult;
}

wstring GetFinalExtractionPath(const StorageObject* storage, const ContentTreeNode* item, const wchar_t* baseDir, int keepPathOpt)
{
	wstring strResult(baseDir);
	if (strResult[strResult.length() - 1] != '\\')
		strResult.append(L"\\");

	ContentTreeNode* pSubRoot = NULL;
	if (keepPathOpt == KPV_PARTIAL)
		pSubRoot = storage->CurrentDir();
	else if (keepPathOpt == KPV_NOPATH)
		pSubRoot = item->parent;

	size_t nSubPathSize = item->GetPath(NULL, 0, pSubRoot) + 1;
	wchar_t *wszItemSubPath = (wchar_t *) malloc(nSubPathSize * sizeof(wchar_t));
	item->GetPath(wszItemSubPath, nSubPathSize, pSubRoot);

	strResult.append(wszItemSubPath);
	free(wszItemSubPath);

	return strResult;
}

void IncludeTrailingPathDelim(wchar_t *pathBuf, size_t bufMaxSize)
{
	size_t nPathLen = wcslen(pathBuf);
	if (pathBuf[nPathLen - 1] != '\\')
		wcscat_s(pathBuf, bufMaxSize, L"\\");
}

void CutFileNameFromPath(wchar_t* fullPath, bool includeTrailingDelim)
{
	wchar_t* lastSlashPos = wcsrchr(fullPath, '\\');
	if (lastSlashPos)
	{
		if (includeTrailingDelim)
			*(lastSlashPos + 1) = 0;
		else
			*lastSlashPos = 0;
	}
	else
	{
		*fullPath = 0;
	}
}
