// wcx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "wcx_loader.h"
#include "OptionsParser.h"
#include "ModuleCRT.h"

static wchar_t optWcxLocation[MAX_PATH] = {0};
static bool optRecursiveLoad = true;

static WcxLoader* g_Loader = NULL;
extern HMODULE g_hDllHandle;

struct WcxStorage
{
	std::wstring FilePath;
	IWcxModule* Module;

	HANDLE ArcHandle;
	int AtItem;
	
	bool ListingComplete;
	std::vector<StorageItemInfo> Items;
	
	WcxStorage() : ArcHandle(NULL), Module(NULL), ListingComplete(false), AtItem(0) {}
};

static void GetDefaultWcxLocation()
{
	memset(optWcxLocation, 0, sizeof(optWcxLocation));
	
	GetModuleFileName(g_hDllHandle, optWcxLocation, MAX_PATH);
	wchar_t* lastDelim = wcsrchr(optWcxLocation, '\\');
	*(lastDelim + 1) = 0;
	wcscat_s(optWcxLocation, MAX_PATH, L"wcx\\");
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	IWcxModule* procModule = NULL;
	HANDLE hArchive = NULL;

	for (size_t i = 0; i < g_Loader->Modules.size(); i++)
	{
		IWcxModule* nextModule = g_Loader->Modules[i];
		if (nextModule->IsArchive(params.FilePath))
		{
			hArchive = nextModule->OpenArchive(params.FilePath, PK_OM_EXTRACT);
			if (hArchive != NULL)
			{
				procModule = nextModule;
				break;
			}
		}
	} // for

	if (procModule != NULL && hArchive != NULL)
	{
		WcxStorage* storeObj = new WcxStorage();
		storeObj->Module = procModule;
		storeObj->ArcHandle = hArchive;
		storeObj->AtItem = 0;
		storeObj->ListingComplete = false;

		*storage = storeObj;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, procModule->GetModuleName());
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");
	}

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	WcxStorage* storeObj = (WcxStorage*) storage;
	if (storeObj == NULL) return;

	storeObj->Items.clear();
	if (storeObj->ArcHandle != NULL)
	{
		storeObj->Module->CloseArchive(storeObj->ArcHandle);
	}
	delete storeObj;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	WcxStorage* storeObj = (WcxStorage*) storage;
	if (storeObj == NULL) return GET_ITEM_ERROR;

	/*
	if (!storeObj->ListingComplete)
	{
		// We are not on first item - reopen archive 
		if (storeObj->ArcHandle != NULL && storeObj->AtItem != 0)
		{
			storeObj->Module->WcsCloseArchive(storeObj->ArcHandle);
			storeObj->ArcHandle = NULL;
		}
	}
	*/
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	WcxStorage* storeObj = (WcxStorage*) storage;
	if (storeObj == NULL) return SER_ERROR_SYSTEM;
	
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

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"WcxLocation", optWcxLocation, sizeof(optWcxLocation) / sizeof(optWcxLocation[0]));
	opts.GetValue(L"RecursiveLoad", optRecursiveLoad);

	if (!optWcxLocation[0])
	{
		GetDefaultWcxLocation();
	}

	g_Loader = new WcxLoader();
	int numModules = g_Loader->LoadModules(optWcxLocation, optRecursiveLoad);

	return (numModules > 0) ? TRUE : FALSE;
}

void MODULE_EXPORT UnloadSubModule()
{
	g_Loader->UnloadModules();
	delete g_Loader;
}
