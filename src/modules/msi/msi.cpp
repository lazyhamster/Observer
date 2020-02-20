// msi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "MsiViewer.h"

#include <mspack.h>

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	CMsiViewer *view = new CMsiViewer();
	int nOpenRes = view->Open(params.FilePath, MSI_OPENFLAG_SHOWSPECIALS);
	if (nOpenRes == ERROR_SUCCESS)
	{
		*storage = view;

		switch (view->GetFileType())
		{
			case MsiFileType::MSI:
				wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MSI Installation Database");
				break;
			case MsiFileType::MergeModule:
				wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MSI Merge Module");
				break;
			default:
				wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Unknown MSI type");
				break;
		}
		
		switch (view->GetCompressionType())
		{
			case MSI_COMPRESSION_NONE:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
				break;
			case MSI_COMPRESSION_CAB:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"MSCab");
				break;
			case MSI_COMPRESSION_MIXED:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Mixed");
				break;
			default:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");
				break;
		}
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
		info->Created = view->GetCreateDateTime();

		return SOR_SUCCESS;
	}

	delete view;
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (view)
		delete view;
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return GET_ITEM_ERROR;

	if (view->FindNodeDataByIndex(item_index, item_info))
		return GET_ITEM_OK;
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return SER_ERROR_SYSTEM;

	FileNode *file = view->GetFile(params.ItemIndex);
	if (!file) return SER_ERROR_SYSTEM;

	int nDumpResult = view->DumpFileContent(file, params.DestPath, params.Callbacks);
	return nDumpResult;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {32D5AEAA-C662-4FA3-AFD1-FFDB64D3B394}
static const GUID MODULE_GUID = { 0x32d5aeaa, 0xc662, 0x4fa3, { 0xaf, 0xd1, 0xff, 0xdb, 0x64, 0xd3, 0xb3, 0x94 } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	int selftest;
	MSPACK_SYS_SELFTEST(selftest);
	if (selftest != MSPACK_ERR_OK)
		return FALSE;
	
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
