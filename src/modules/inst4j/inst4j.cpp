// inst4j.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include "Install4jFile.h"


int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	if (!SignatureMatchOrNull(params.Data, params.DataSize, FILE_SIGNATURE_EXE))
		return SOR_INVALID_FILE;

	CInstall4jFile* arc = new CInstall4jFile();
	if (!arc->Open(params.FilePath))
	{
		delete arc;
		return SOR_INVALID_FILE;
	}

	*storage = arc;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Install4j");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"TODO");
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	
	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	CInstall4jFile* arc = static_cast<CInstall4jFile*>(storage);
	if (arc) delete arc;
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CInstall4jFile* arc = static_cast<CInstall4jFile*>(storage);
	if (!arc || (item_index < 0)) return GET_ITEM_ERROR;

	if (item_index < static_cast<int>(arc->NumArcs()))
	{
		DataArchiveInfo arcInfo = arc->GetArcInfo(item_index);

		MultiByteToWideChar(CP_UTF8, 0, arcInfo.Name.c_str(), -1, item_info->Path, STRBUF_SIZE(item_info->Path));
		item_info->Size = arcInfo.Size;
		item_info->PackedSize = arcInfo.Size;

		return GET_ITEM_OK;
	}

	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CInstall4jFile* arc = static_cast<CInstall4jFile*>(storage);
	if (!arc || (params.ItemIndex < 0)) return SER_ERROR_SYSTEM;

	if (arc->ExtractArc(params.ItemIndex, params.DestPath))
		return SER_SUCCESS;
	
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

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
