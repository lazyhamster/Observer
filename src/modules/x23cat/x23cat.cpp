// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include "x2fd/x2fd.h"
#include "x2fd/common/ext_list.h"

struct XStorage
{
	X2CATALOG Catalog;
	ext::list<X2CATFILEINFO> Files;
};

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

bool CheckDataExists(const wchar_t* catPath)
{
	wchar_t *wszDatPath	= _wcsdup(catPath);
	wcscpy_s(wszDatPath + wcslen(wszDatPath) - 4, 5, L".dat");
	bool fResult = FileExists(wszDatPath);
	free(wszDatPath);

	return fResult;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	// Check extension first
	size_t nPathLen = wcslen(path);
	if ((nPathLen < 5) || _wcsicmp(path + nPathLen - 4, L".cat") || !FileExists(path))
		return FALSE;
	
	// Check corresponding .dat file
	if (!CheckDataExists(path))
		return FALSE;

	char tmp[MAX_PATH] = {0};
	WideCharToMultiByte(CP_ACP, 0, path, wcslen(path), tmp, MAX_PATH, NULL, NULL);
	
	X2CATALOG hCat = X2FD_OpenCatalog(tmp, X2FD_OPEN_EXISTING);
	if (hCat != 0)
	{
		XStorage* xst = new XStorage();
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
		wcscpy_s(info->Compression, STORAGE_FORMAT_NAME_MAX_LEN, L"");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"");

		return TRUE;
	}
		
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	if (storage != NULL)
	{
		XStorage* xst = (XStorage*) storage;
		
		X2FD_CloseCatalog(xst->Catalog);
		xst->Files.clear();
		delete xst;
	}
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
}
