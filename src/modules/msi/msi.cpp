// msi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "MsiViewer.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	CMsiViewer *view = new CMsiViewer();
	int nOpenRes = view->Open(params.FilePath, MSI_OPENFLAG_SHOWSPECIALS);
	if (nOpenRes == ERROR_SUCCESS)
	{
		*storage = (INT_PTR *) view;

		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MSI");
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

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return GET_ITEM_ERROR;

	if (view->FindNodeDataByIndex(item_index, item_data, item_path, path_size))
		return GET_ITEM_OK;
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return SER_ERROR_SYSTEM;

	FileNode *file = view->GetFile(params.item);
	if (!file) return SER_ERROR_SYSTEM;

	int nDumpResult = view->DumpFileContent(file, params.destFilePath, params.callbacks);
	return nDumpResult;
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
