// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

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
	
	wchar_t* oldExt = GetFileExt(originalPath);
	if (wcscmp(oldExt, L".pck") == 0)
	{
		char buf[20] = {0};
		
		X2FD_SeekFile(xf, 0, X2FD_SEEK_SET);
		X2FDLONG ret = X2FD_ReadFile(xf, buf, sizeof(buf));

		if (ret >= 0)
		{
			if (strstr(buf, "<?xml") != NULL)
				return L".xml";
			else if (strncmp(buf, "DDS", 3) == 0)
				return L".dds";
		}

		return L".txt";
	}
	else if (wcscmp(oldExt, L".pbd") == 0)
		return L".bod";
	else if (wcscmp(oldExt, L".pbb") == 0)
		return L".bob";
	
	return oldExt;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	// Check extension first
	const wchar_t* fileExt = GetFileExt(params.FilePath);
	if (!fileExt || (wcslen(fileExt) != 4)) return SOR_INVALID_FILE;

	if (_wcsicmp(fileExt, L".cat") == 0)
	{
		x2catbuffer* hCat = new x2catbuffer();
		if (hCat->open(params.FilePath))
		{
			XStorage* xst = new XStorage();
			xst->Path = _wcsdup(params.FilePath);
			xst->IsCatalog = true;
			xst->Catalog = hCat;

			*storage = xst;
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"X-CAT");
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
			//wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"");

			return SOR_SUCCESS;
		}
		else
			delete hCat;
	}
	else if ((_wcsicmp(fileExt, L".pck") == 0) || (_wcsicmp(fileExt, L".pbd") == 0) || (_wcsicmp(fileExt, L".pbb") == 0))
	{
		X2FILE hFile = X2FD_OpenFile(params.FilePath, X2FD_READ, X2FD_OPEN_EXISTING, X2FD_FILETYPE_AUTO);
		if (hFile != 0)
		{
			XStorage* xst = new XStorage();
			xst->Path = _wcsdup(params.FilePath);
			xst->IsCatalog = false;
			xst->FilePtr = hFile;

			*storage = xst;
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

			return SOR_SUCCESS;
		}
	}
			
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
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

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	if ((storage == NULL) || (item_index < 0) || (item_info == NULL))
		return GET_ITEM_ERROR;

	XStorage* xst = (XStorage*) storage;
	if (xst->IsCatalog)
	{
		if (item_index >= (int) xst->Catalog->size())
			return GET_ITEM_NOMOREITEMS;

		x2catentry* entry = GetCatEntryByIndex(xst->Catalog, item_index);
		if (entry == NULL) return GET_ITEM_ERROR;

		memset(item_info, 0, sizeof(StorageItemInfo));
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), entry->pszFileName);
		item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
		item_info->Size = entry->size;

		return GET_ITEM_OK;
	}
	else if (item_index == 0)	// Files other then catalogs always has one file inside
	{
		X2FILEINFO finfo;
		if (X2FD_FileStatByHandle(xst->FilePtr, &finfo) == 0)
			return GET_ITEM_ERROR;

		memset(item_info, 0, sizeof(StorageItemInfo));
		item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
		item_info->Size = finfo.size;
		item_info->PackedSize = finfo.BinarySize;

		const wchar_t* fileName = GetFileName(xst->Path);
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), fileName);

		// Change extension
		wchar_t *oldExt = GetFileExt(item_info->Path);
		if (oldExt)
		{
			*oldExt = 0;
			wcscat_s(item_info->Path, STRBUF_SIZE(item_info->Path), GetInternalFileExt(xst->FilePtr, xst->Path));
		}
		
		if (finfo.mtime != -1)
		{
			FILETIME ftMTime;
			UnixTimeToFileTime(finfo.mtime, &ftMTime);
			FileTimeToLocalFileTime(&ftMTime, &item_info->ModificationTime);
		}

		return GET_ITEM_OK;
	}

	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	if ((storage == NULL) || (params.ItemIndex < 0))
		return SER_ERROR_SYSTEM;

	XStorage* xst = (XStorage*) storage;
	if (xst->IsCatalog)
	{
		if (params.ItemIndex >= (int) xst->Catalog->size())
			return SER_ERROR_SYSTEM;

		x2catentry* entry = GetCatEntryByIndex(xst->Catalog, params.ItemIndex);
		if (entry == NULL) return GET_ITEM_ERROR;

		X2FILE hInput = X2FD_OpenFileInCatalog(xst->Catalog, entry, X2FD_FILETYPE_PLAIN);
		if (hInput == 0) return SER_ERROR_READ;

		X2FILE hOutput = X2FD_OpenFile(params.DestPath, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
		if (hOutput == 0)
		{
			X2FD_CloseFile(hInput);
			return SER_ERROR_WRITE;
		}

		bool res = X2FD_CopyFile(hInput, hOutput);
		
		X2FD_CloseFile(hInput);
		X2FD_CloseFile(hOutput);

		if (res) return SER_SUCCESS;
	}
	else if (params.ItemIndex == 0)
	{
		// Append name
		X2FILEINFO finfo = {0};
		X2FD_FileStatByHandle(xst->FilePtr, &finfo);
		
		X2FILE hOutput = X2FD_OpenFile(params.DestPath, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
		if (hOutput == 0) return SER_ERROR_WRITE;

		bool res = X2FD_CopyFile(xst->FilePtr, hOutput);
		X2FD_CloseFile(hOutput);

		if (res) return SER_SUCCESS;
	}

	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
