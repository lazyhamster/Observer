// relic.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include "base\HWClassFactory.h"

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	CHWAbstractStorage* fileObj = CHWClassFactory::LoadFile(params.FilePath);
	if (fileObj != NULL)
	{
		*storage = fileObj;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, fileObj->GetFormatName());
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, fileObj->GetLabel());
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, fileObj->GetCompression());

		return SOR_SUCCESS;
	}

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	if (storage)
	{
		CHWAbstractStorage* fileObj = (CHWAbstractStorage*) storage;
		delete fileObj;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CHWAbstractStorage* fileObj = (CHWAbstractStorage*) storage;
	if (!fileObj) return GET_ITEM_ERROR;
	
	if (item_index < 0) return GET_ITEM_ERROR;
	if (item_index >= fileObj->NumFiles()) return GET_ITEM_NOMOREITEMS;

	HWStorageItem item = {0};
	if (fileObj->GetFileInfo(item_index, &item))
	{
		memset(item_info, 0, sizeof(StorageItemInfo));

		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), item.Name);
		item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
		item_info->Size = item.UncompressedSize;
		item_info->PackedSize = item.CompressedSize;
		UnixTimeToFileTime(item.ModTime, &item_info->ModificationTime);

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_ERROR;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CHWAbstractStorage* fileObj = (CHWAbstractStorage*) storage;
	if (!fileObj) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= fileObj->NumFiles())
		return SER_ERROR_SYSTEM;

	HWStorageItem item = {0};
	if (fileObj->GetFileInfo(params.item, &item))
	{
		HANDLE hOutputFile = CreateFile(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hOutputFile == INVALID_HANDLE_VALUE)
			return SER_ERROR_WRITE;

		bool fOpRes = fileObj->ExtractFile(params.item, hOutputFile);

		if (fOpRes && item.ModTime)
		{
			FILETIME ft = {0};
			UnixTimeToFileTime(item.ModTime, &ft);
			SetFileTime(hOutputFile, NULL, NULL, &ft);
		}

		CloseHandle(hOutputFile);

		if (!fOpRes)
		{
			DeleteFile(params.destFilePath);
			return SER_ERROR_READ;
		}

		return SER_SUCCESS;
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
