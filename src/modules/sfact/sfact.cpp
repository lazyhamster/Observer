// sfact.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "SfFile.h"


int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	SetupFactoryFile* sfInst = OpenInstaller(params.FilePath);
	if (sfInst != nullptr)
	{
		*storage = sfInst;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Setup Factory Installer");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
		
		return SOR_SUCCESS;
	}

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst != NULL)
	{
		FreeInstaller(sfInst);
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst == NULL || item_index < 0) return GET_ITEM_ERROR;
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst == NULL || params.item < 0) return SER_ERROR_SYSTEM;
	
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

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
