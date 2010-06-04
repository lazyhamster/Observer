// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include "x2fd/x2fd.h"
#include "x2fd/common.h"
#include "x2fd/common/ext_list.h"

struct XStorage
{
	bool IsCatalog;
	wchar_t* Path;
	
	X2FILE FilePtr;		// User for .pck files
	x2catbuffer* Catalog;	// User for .cat/.dat pair

	XStorage()
	{
		FilePtr = 0;
		Catalog = NULL;
		Path = NULL;
	}
};

static x2catentry* GetCatEntryByIndex(x2catbuffer* catalog, int index)
{
	int cnt = 0;
	x2catbuffer::iterator it = catalog->begin();
	while ((cnt < index) && (it != catalog->end()))
	{
		cnt++;
		it++;
	}
	if (cnt == index)
	{
		x2catentry* entry = *it;
		return entry;
	}
	return NULL;
}

static wchar_t* GetInternalFileExt(X2FILE xf, wchar_t* originalPath)
{
	if (xf == 0) return L"";
	
	wchar_t* oldExt = wcsrchr(originalPath, '.');
	if (wcscmp(oldExt, L".pck") == 0)
	{
		char buf[20] = {0};
		
		X2FD_SeekFile(xf, 0, X2FD_SEEK_SET);
		X2FDLONG ret = X2FD_ReadFile(xf, buf, sizeof(buf));

		if ((ret >= 0) && strstr(buf, "<?xml"))
			return L".xml";
		else
			return L".txt";
	}
	else if (wcscmp(oldExt, L".pbd") == 0)
		return L".bod";
	else if (wcscmp(oldExt, L".pbb") == 0)
		return L".bob";
	
	return oldExt;
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
		x2catbuffer* hCat = new x2catbuffer();
		if (hCat->open(tmp))
		{
			XStorage* xst = new XStorage();
			xst->Path = _wcsdup(path);
			xst->IsCatalog = true;
			xst->Catalog = hCat;

			*storage = (INT_PTR*) xst;
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"X-CAT");
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
			//wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"");

			return TRUE;
		}
		else
			delete hCat;
	}
	else if ((_wcsicmp(fileExt, L".pck") == 0) || (_wcsicmp(fileExt, L".pbd") == 0) || (_wcsicmp(fileExt, L".pbb") == 0))
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
			
			int nCompression = X2FD_GetFileCompressionType(hFile);
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
			delete xst->Catalog;
		else
			X2FD_CloseFile(xst->FilePtr);
		
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
		if (item_index >= (int) xst->Catalog->size())
			return GET_ITEM_NOMOREITEMS;

		x2catentry* entry = GetCatEntryByIndex(xst->Catalog, item_index);
		if (entry == NULL) return GET_ITEM_ERROR;

		memset(item_path, 0, path_size * sizeof(wchar_t));
		MultiByteToWideChar(CP_ACP, 0, entry->pszFileName, strlen(entry->pszFileName), item_path, path_size);

		const wchar_t* fileName = GetFileName(item_path);

		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		wcscpy_s(item_data->cFileName, MAX_PATH, fileName);
		wcscpy_s(item_data->cAlternateFileName, 14, L"");
		item_data->nFileSizeLow = (DWORD) entry->size;

		return GET_ITEM_OK;
	}
	else if (item_index == 0)	// Files other then catalogs always has one file inside
	{
		X2FILEINFO finfo;
		if (X2FD_FileStatByHandle(xst->FilePtr, &finfo) == 0)
			return GET_ITEM_ERROR;

		const wchar_t* fileName = GetFileName(xst->Path);
		wcscpy_s(item_path, path_size, fileName);

		// Change extension
		wchar_t *oldExt = GetFileExt(item_path);
		if (oldExt)
		{
			*oldExt = 0;
			wcscat_s(item_path, path_size, GetInternalFileExt(xst->FilePtr, xst->Path));
		}

		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		wcscpy_s(item_data->cFileName, MAX_PATH, item_path);
		wcscpy_s(item_data->cAlternateFileName, 14, L"");
		item_data->nFileSizeLow = finfo.size;
		
		if (finfo.mtime != -1)
		{
			FILETIME ftMTime;
			UnixTimeToFileTime(finfo.mtime, &ftMTime);
			FileTimeToLocalFileTime(&ftMTime, &item_data->ftLastWriteTime);
		}

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
		if (params.item >= (int) xst->Catalog->size())
			return GET_ITEM_NOMOREITEMS;

		x2catentry* entry = GetCatEntryByIndex(xst->Catalog, params.item);
		if (entry == NULL) return GET_ITEM_ERROR;

		X2FILE hInput = X2FD_OpenFileInCatalog(xst->Catalog, entry, X2FD_FILETYPE_PLAIN);
		if (hInput == 0) return SER_ERROR_READ;

		char szDest[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, params.dest_path, wcslen(params.dest_path), szDest, MAX_PATH, NULL, NULL);
		strcat_s(szDest, MAX_PATH, GetFileName(entry->pszFileName));
		
		X2FILE hOutput = X2FD_OpenFile(szDest, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
		if (hOutput == 0)
		{
			X2FD_CloseFile(hInput);
			return SER_ERROR_WRITE;
		}

		int res = X2FD_CopyFile(hInput, hOutput);
		
		X2FD_CloseFile(hInput);
		X2FD_CloseFile(hOutput);

		pctx->nProcessedBytes += entry->size;
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
		char* curExt = strrchr(tmp, '.');
		const wchar_t* intExt = GetInternalFileExt(xst->FilePtr, xst->Path);
		if (intExt && curExt)
			WideCharToMultiByte(CP_ACP, 0, intExt, wcslen(intExt), curExt, strlen(curExt) + 1, NULL, NULL);
		
		X2FILE hOutput = X2FD_OpenFile(tmp, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
		if (hOutput == 0) return SER_ERROR_WRITE;

		int res = X2FD_CopyFile(xst->FilePtr, hOutput);
		X2FD_CloseFile(hOutput);

		pctx->nProcessedBytes += finfo.size;
		if (res > 0) return SER_SUCCESS;
	}

	return SER_ERROR_SYSTEM;
}
