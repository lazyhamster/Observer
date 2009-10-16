// msi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\ModuleDef.h"
#include "MsiViewer.h"

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	CMsiViewer *view = new CMsiViewer();
	int nOpenRes = view->Open(path, false);
	if (nOpenRes == ERROR_SUCCESS)
	{
		*storage = (INT_PTR *) view;

		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MSI");
		wcscpy_s(info->SubType, STORAGE_SUBTYPE_NAME_MAX_LEN, L"-");
		info->NumRealItems = view->GetTotalFiles() + view->GetTotalDirectories();

		return TRUE;
	}

	delete view;
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (view)
		delete view;
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return FALSE;

	if (view->FindNodeDataByIndex(item_index, item_data, item_path, path_size))
		return TRUE;
	
	return FALSE;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return SER_ERROR_SYSTEM;

	FileNode *file = view->GetFile(params.item);
	if (!file) return SER_ERROR_SYSTEM;

	ProgressContext* pctx = (ProgressContext*) params.callbacks.signalContext;
	pctx->nCurrentFileProgress = 0;
	pctx->nCurrentFileIndex = params.item;

	params.callbacks.FileStart(pctx);
	int nDumpResult = view->DumpFileContent(file, params.dest_path, params.callbacks);
	pctx->nProcessedBytes += file->GetSize();
	params.callbacks.FileEnd(pctx);

	return nDumpResult;
}
