// ishield.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ISCabFile.h"

static const char* BOM = "\xFF\xFE";

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	ISCabFile* cabStorage = OpenCab(params.FilePath);
	if (cabStorage != NULL)
	{
		*storage = cabStorage;
		
		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Install Shield Cabinet");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"ZData");
		
		return SOR_SUCCESS;
	}
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	ISCabFile* cabStorage = (ISCabFile*) storage;
	if (cabStorage != NULL)
	{
		CloseCab(cabStorage);
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	ISCabFile* cabStorage = (ISCabFile*) storage;
	if (cabStorage == NULL) return GET_ITEM_ERROR;

	memset(item_info, 0, sizeof(StorageItemInfo));
	if (item_index == 0)
	{
		// First file will be always fake info file
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{info}.txt");
		item_info->Size = cabStorage->GetCabInfo().size() * sizeof(wchar_t);
		return GET_ITEM_OK;
	}
	else if (item_index > 0 && item_index < cabStorage->GetTotalFiles())
	{
		if (cabStorage->GetFileInfo(item_index - 1, item_info))
			return GET_ITEM_OK;
		else
			return GET_ITEM_ERROR;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	ISCabFile* cabStorage = (ISCabFile*) storage;
	if (cabStorage == NULL) return SER_ERROR_SYSTEM;

	HANDLE hFile = CreateFile(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	int status = SER_SUCCESS;
	if (params.item == 0)
	{
		// Dump info file
		DWORD dwBytes;
		const std::wstring infoData = cabStorage->GetCabInfo();
		WriteFile(hFile, BOM, strlen(BOM), &dwBytes, NULL);
		WriteFile(hFile, infoData.c_str(), infoData.size() * sizeof(wchar_t), &dwBytes, NULL);
	}
	else if (params.item > 0 && params.item < cabStorage->GetTotalFiles())
	{
		// Dump archive file
		status = cabStorage->ExtractFile(params.item - 1, hFile) ? SER_SUCCESS : SER_ERROR_READ;
	}

	CloseHandle(hFile);
	if (status != SER_SUCCESS)
	{
		DeleteFile(params.destFilePath);
	}
	return status;
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
