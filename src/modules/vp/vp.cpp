// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "vp_file.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	// Open container
	CVPFile* file = new CVPFile();
	if (file->Open(params.FilePath))
	{
		*storage = file;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Volition Pack");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"Version 2");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

		return SOR_SUCCESS;
	}
	
	delete file;
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	if (storage)
	{
		CVPFile *file = (CVPFile*) storage;
		delete file;
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CVPFile* file = (CVPFile*) storage;
	if (!file) return GET_ITEM_ERROR;

	if ((item_index < 0) || (item_index >= (int) file->ItemCount()))
		return GET_ITEM_NOMOREITEMS;

	VP_FileRec frec;
	if (!file->GetItem(item_index, frec))
		return GET_ITEM_ERROR;

	memset(item_info, 0, sizeof(StorageItemInfo));
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), frec.full_path);
	item_info->Attributes = (frec.IsDir()) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	item_info->Size = frec.size;
	item_info->ModificationTime = frec.timestamp;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CVPFile* file = (CVPFile*) storage;
	if (!file) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= (int) file->ItemCount())
		return SER_ERROR_SYSTEM;

	return file->ExtractSingleFile(params.item, params.destFilePath, &(params.callbacks));
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
