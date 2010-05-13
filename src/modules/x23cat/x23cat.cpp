// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include "x2fd/x2fd.h"
#include "x2fd/common/ext_list.h"

struct XStorage
{
	bool IsCatalog;
	wchar_t* Path;
	
	X2FILE FilePtr;		// User for .pck files
	X2CATALOG Catalog;	// User for .cat/.dat pair
	ext::list<X2CATFILEINFO> Files;

	bool GetItemByIndex(int index, X2CATFILEINFO &fileInfo)
	{
		if (!IsCatalog) return false;
		
		int cnt = 0;
		ext::list<X2CATFILEINFO>::iterator it = Files.begin();
		while ((cnt < index) && (it != Files.end()))
		{
			cnt++;
			it++;
		}
		if (cnt == index)
		{
			fileInfo = *it;
			return true;
		}
		return false;
	}
};

//////////////////////////////////////////////////////////////////////////

static bool FileExists(const wchar_t* path)
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

static void UnixTimeToFileTime(X2FDLONG t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

static const char* GetFileName(const char* path)
{
	const char* fileName = strrchr(path, '\\');
	if (fileName)
		return ++fileName;
	else
		return path;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	// Check extension first
	const wchar_t* fileExt = wcsrchr(path, '.');
	if (!fileExt || (wcslen(fileExt) != 4)) return FALSE;

	char tmp[MAX_PATH] = {0};
	WideCharToMultiByte(CP_ACP, 0, path, wcslen(path), tmp, MAX_PATH, NULL, NULL);

	if (_wcsicmp(fileExt, L".cat") == 0)
	{
		X2CATALOG hCat = X2FD_OpenCatalog(tmp, X2FD_OPEN_EXISTING);
		if (hCat != 0)
		{
			XStorage* xst = new XStorage();
			xst->Path = _wcsdup(path);
			xst->IsCatalog = true;
			xst->Catalog = hCat;

			// Do files listing		
			X2CATFILEINFO finfo;
			X2FIND hFind = X2FD_CatFindFirstFile(hCat, "*", &finfo);
			if(hFind){
				do{
					xst->Files.push_back(finfo);
				}
				while(X2FD_CatFindNextFile(hFind, &finfo) !=0 );
			}
			X2FD_CatFindClose(hFind);

			*storage = (INT_PTR*) xst;
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"X-CAT");
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
			//wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"");

			return TRUE;
		}
	}
	else if (_wcsicmp(fileExt, L".pck") == 0)
	{
		X2FILE hFile = X2FD_OpenFile(tmp, X2FD_READ, X2FD_OPEN_EXISTING, X2FD_FILETYPE_AUTO);
		if (hFile != 0)
		{
			XStorage* xst = new XStorage();
			xst->Path = _wcsdup(path);
			xst->IsCatalog = false;
			xst->FilePtr = hFile;

			*storage = (INT_PTR*) xst;
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"X-PCK");
			//wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"");
			
			int nCompression = X2FD_GetFileCompressionType(tmp);
			switch (nCompression)
			{
				case X2FD_FILETYPE_PCK:
					wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"PCK");
					break;
				case X2FD_FILETYPE_DEFLATE:
					wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Deflate");
					break;
				default:
					wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Plain");
					break;
			}

			return TRUE;
		}
	}
			
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	if (storage != NULL)
	{
		XStorage* xst = (XStorage*) storage;
		
		free(xst->Path);
		if (xst->IsCatalog)
		{
			X2FD_CloseCatalog(xst->Catalog);
			xst->Files.clear();
		}
		else
		{
			X2FD_CloseFile(xst->FilePtr);
		}
		
		delete xst;
	}
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	if ((storage == NULL) || (item_index < 0) || (item_data == NULL))
		return GET_ITEM_ERROR;

	XStorage* xst = (XStorage*) storage;
	if (xst->IsCatalog)
	{
		if (item_index >= (int) xst->Files.size())
			return GET_ITEM_NOMOREITEMS;

		X2CATFILEINFO finfo;
		if (!xst->GetItemByIndex(item_index, finfo))
			return GET_ITEM_ERROR;

		memset(item_path, 0, path_size * sizeof(wchar_t));
		MultiByteToWideChar(CP_ACP, 0, finfo.szFileName, strlen(finfo.szFileName), item_path, path_size);

		const wchar_t* fileName = wcsrchr(item_path, L'\\');
		if (fileName == NULL)
			fileName = item_path;
		else
			fileName++;

		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		wcscpy_s(item_data->cFileName, MAX_PATH, fileName);
		wcscpy_s(item_data->cAlternateFileName, 14, L"");
		item_data->nFileSizeLow = finfo.size;

		return GET_ITEM_OK;
	}
	else if (item_index == 0)	// Files other then catalogs always has one file inside
	{
		X2FILEINFO finfo;
		if (X2FD_FileStatByHandle(xst->FilePtr, &finfo) == 0)
			return GET_ITEM_ERROR;

		const char* fileName = GetFileName(finfo.szFileName);

		memset(item_path, 0, path_size * sizeof(wchar_t));
		MultiByteToWideChar(CP_ACP, 0, fileName, strlen(fileName), item_path, path_size);

		// Change extension
		size_t nNameLen = wcslen(item_path);
		if (nNameLen > 4)
			wcscpy_s(item_path + nNameLen - 4, 5, L".txt");

		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		wcscpy_s(item_data->cFileName, MAX_PATH, item_path);
		wcscpy_s(item_data->cAlternateFileName, 14, L"");
		item_data->nFileSizeLow = finfo.size;
		
		X2FDLONG localTime = X2FD_TimeStampToLocalTimeStamp(finfo.mtime);
		UnixTimeToFileTime(localTime, &item_data->ftLastWriteTime);

		return GET_ITEM_OK;
	}

	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	if ((storage == NULL) || (params.item < 0))
		return SER_ERROR_SYSTEM;

	ProgressContext* pctx = (ProgressContext*) params.callbacks.signalContext;

	XStorage* xst = (XStorage*) storage;
	if (xst->IsCatalog)
	{
		if (params.item >= (int) xst->Files.size())
			return GET_ITEM_NOMOREITEMS;

		X2CATFILEINFO finfo;
		if (!xst->GetItemByIndex(params.item, finfo))
			return GET_ITEM_ERROR;
		
		char szSrc[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, xst->Path, wcslen(xst->Path), szSrc, MAX_PATH, NULL, NULL);
		strcat_s(szSrc, MAX_PATH, "::");
		strcat_s(szSrc, MAX_PATH, finfo.szFileName);
		
		X2FILE hInput = X2FD_OpenFile(szSrc, X2FD_READ, X2FD_OPEN_EXISTING, X2FD_FILETYPE_PLAIN);
		if (hInput == 0) return SER_ERROR_READ;

		char szDest[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, params.dest_path, wcslen(params.dest_path), szDest, MAX_PATH, NULL, NULL);
		strcat_s(szDest, MAX_PATH, GetFileName(finfo.szFileName));
		
		X2FILE hOutput = X2FD_OpenFile(szDest, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
		if (hOutput == 0)
		{
			X2FD_CloseFile(hInput);
			return SER_ERROR_WRITE;
		}

		int res = X2FD_CopyFile(hInput, hOutput);
		
		X2FD_CloseFile(hInput);
		X2FD_CloseFile(hOutput);

		pctx->nProcessedBytes += finfo.size;
		if (res > 0) return SER_SUCCESS;
	}
	else if (params.item == 0)
	{
		char tmp[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, params.dest_path, wcslen(params.dest_path), tmp, MAX_PATH, NULL, NULL);

		// Append name
		X2FILEINFO finfo = {0};
		X2FD_FileStatByHandle(xst->FilePtr, &finfo);
		strcat_s(tmp, MAX_PATH, GetFileName(finfo.szFileName));
		
		// Change extension
		size_t nNameLen = strlen(tmp);
		if (nNameLen > 4)
			strcpy_s(tmp + nNameLen - 4, 5, ".txt");
		
		X2FILE hOutput = X2FD_OpenFile(tmp, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
		if (hOutput == 0) return SER_ERROR_WRITE;

		int res = X2FD_CopyFile(xst->FilePtr, hOutput);
		X2FD_CloseFile(hOutput);

		pctx->nProcessedBytes += finfo.size;
		if (res > 0) return SER_SUCCESS;
	}

	return SER_ERROR_SYSTEM;
}
