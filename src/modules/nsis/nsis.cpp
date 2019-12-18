// nsis.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "NsisArchive.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	CNsisArchive* arc = new CNsisArchive();
	if (!arc->Open(params.FilePath))
	{
		delete arc;
		return SOR_INVALID_FILE;
	}

	*storage = arc;
	
	memset(info, 0, sizeof(StorageGeneralInfo));
	arc->GetSubtypeName(info->Format, STORAGE_FORMAT_NAME_MAX_LEN);
	arc->GetCompressionName(info->Compression, STORAGE_PARAM_MAX_LEN);
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");

	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	CNsisArchive* arc = (CNsisArchive *) storage;
	if (arc)
		delete arc;
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CNsisArchive* arc = (CNsisArchive *) storage;
	if (!arc) return GET_ITEM_ERROR;
	
	if (item_index < 0)
		return GET_ITEM_ERROR;

	if (item_index >= arc->GetItemsCount())
		return GET_ITEM_NOMOREITEMS;

	int nRes = arc->GetItem(item_index, item_info);
	return (nRes) ? GET_ITEM_OK : GET_ITEM_ERROR;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CNsisArchive* arc = (CNsisArchive *) storage;
	if (!arc) return SER_ERROR_SYSTEM;
	
	return arc->ExtractArcItem(params.ItemIndex, params.DestPath, &(params.Callbacks));
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {8A6011C7-565F-41D2-A5E6-B063330813DB}
static const GUID MODULE_GUID = { 0x8a6011c7, 0x565f, 0x41d2, { 0xa5, 0xe6, 0xb0, 0x63, 0x33, 0x8, 0x13, 0xdb } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
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
