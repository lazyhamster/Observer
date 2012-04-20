// wcx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "wcx_loader.h"

const wchar_t* cntDefaultWcxPath = L".\\wcx";

static WcxLoader* Loader = NULL;

struct WcxStorage
{
	HANDLE ArcHandle;
	WcxModule* Module;
};

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	/*
	*storage = vdObj;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, GetDiskType(vdisk));
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
	*/

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->OpenStorage = OpenStorage;
	LoadParams->CloseStorage = CloseStorage;
	LoadParams->GetItem = GetStorageItem;
	LoadParams->ExtractItem = ExtractItem;

	Loader = new WcxLoader();
	Loader->LoadModules(cntDefaultWcxPath);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
	Loader->UnloadModules();
	delete Loader;
}
