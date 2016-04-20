// wcx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "wcx_loader.h"
#include "modulecrt/OptionsParser.h"
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
	
	std::vector<StorageItemInfo> Items;
	
	WcxStorage() : ArcHandle(NULL), Module(NULL), AtItem(0) {}
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
			hArchive = nextModule->OpenArchive(params.FilePath, PK_OM_LIST);
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
		storeObj->FilePath = params.FilePath;

		*storage = storeObj;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, procModule->GetModuleName());
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

		return SOR_SUCCESS;
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

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	WcxStorage* storeObj = (WcxStorage*) storage;
	if (storeObj == NULL) return FALSE;

	IWcxModule* module = storeObj->Module;
	tHeaderDataExW header;

	do 
	{
		int headRet = module->ReadHeader(storeObj->ArcHandle, &header);
		if (headRet == E_END_ARCHIVE) break;
		if (headRet != 0) return GET_ITEM_ERROR;

		StorageItemInfo nextItem = {0};
		wcscpy_s(nextItem.Path, sizeof(nextItem.Path) / sizeof(nextItem.Path[0]), header.FileName);
		nextItem.Attributes = header.FileAttr;
		nextItem.Size = ((__int64) header.UnpSizeHigh << 32) | (__int64) header.UnpSize;
		nextItem.PackedSize = ((__int64) header.PackSizeHigh << 32) | (__int64) header.PackSize;

		FILETIME filetime;
		DosDateTimeToFileTime ((WORD)((DWORD)header.FileTime >> 16), (WORD)header.FileTime, &filetime);
		LocalFileTimeToFileTime (&filetime, &nextItem.ModificationTime);

		storeObj->Items.push_back(nextItem);

		int procRet = module->ProcessFile(storeObj->ArcHandle, PK_SKIP, NULL, NULL);
		if (procRet != 0 && procRet != E_END_ARCHIVE) return GET_ITEM_ERROR;

	} while (true);

	// Skip pointer past last item to ensure reopen on extract
	storeObj->AtItem = (int) storeObj->Items.size();
	
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	WcxStorage* storeObj = (WcxStorage*) storage;
	if (storeObj == NULL) return GET_ITEM_ERROR;

	if (item_index < (int) storeObj->Items.size())
	{
		StorageItemInfo &cachedItem = storeObj->Items.at(item_index);
		memcpy(item_info, &cachedItem, sizeof(StorageItemInfo));

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	WcxStorage* storeObj = (WcxStorage*) storage;
	if (storeObj == NULL) return SER_ERROR_SYSTEM;

	if (params.ItemIndex < 0 || params.ItemIndex >= (int)storeObj->Items.size())
		return SER_ERROR_SYSTEM;

	// Reopen archive if arch pointer later then requested file
	if (params.ItemIndex < storeObj->AtItem)
	{
		storeObj->Module->CloseArchive(storeObj->ArcHandle);
		storeObj->ArcHandle = storeObj->Module->OpenArchive(storeObj->FilePath.c_str(), PK_OM_EXTRACT);
		storeObj->AtItem = 0;

		if (storeObj->ArcHandle == NULL) return SER_ERROR_READ;
	}

	// Skip until required item
	tHeaderDataExW header = {0};
	while (storeObj->AtItem < params.ItemIndex)
	{
		storeObj->Module->ReadHeader(storeObj->ArcHandle, &header);
		storeObj->Module->ProcessFile(storeObj->ArcHandle, PK_SKIP, NULL, NULL);
		storeObj->AtItem++;
	}

	// Do extraction
	storeObj->Module->ReadHeader(storeObj->ArcHandle, &header);
	int retVal = storeObj->Module->ProcessFile(storeObj->ArcHandle, PK_EXTRACT, NULL, const_cast<wchar_t*>(params.DestPath));
	storeObj->AtItem++;
	
	if (retVal == 0)
		return SER_SUCCESS;
	else if (retVal == E_EOPEN || retVal == E_EWRITE)
		return SER_ERROR_WRITE;
	else
		return SER_ERROR_READ;
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

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"WcxLocation", optWcxLocation, sizeof(optWcxLocation) / sizeof(optWcxLocation[0]));
	opts.GetValue(L"RecursiveLoad", optRecursiveLoad);

	TrimStr(optWcxLocation);
	if (!optWcxLocation[0])
	{
		GetDefaultWcxLocation();
	}
	IncludeTrailingPathDelim(optWcxLocation, sizeof(optWcxLocation) / sizeof(optWcxLocation[0]));

	g_Loader = new WcxLoader();
	g_Loader->LoadModules(optWcxLocation, optRecursiveLoad);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
	g_Loader->UnloadModules();
	delete g_Loader;
}
