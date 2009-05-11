// msi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\ModuleDef.h"
#include "MsiViewer.h"

int MODULE_EXPORT LoadSubModule(int Reserved)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	CMsiViewer *view = new CMsiViewer();
	int nOpenRes = view->Open(path);
	if (nOpenRes == ERROR_SUCCESS)
	{
		*storage = (INT_PTR *) view;

		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MSI");
		info->NumFiles = view->GetTotalFiles();
		info->NumDirectories = view->GetTotalDirectories();
		info->TotalSize = view->GetTotalSize();
		info->NumRealItems = info->NumFiles + info->NumDirectories;

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

int MODULE_EXPORT GetNextItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return FALSE;

	if (view->FindNodeDataByIndex(item_index, item_data, item_path, path_size))
		return TRUE;
	
	return FALSE;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, const wchar_t* item, int Params, const wchar_t* destPath, const ExtractProcessCallbacks* epc)
{
	CMsiViewer *view = (CMsiViewer *) storage;
	if (!view) return SER_ERROR_SYSTEM;

	FileNode *file = view->GetFile(item);
	if (!file) return SER_ERROR_SYSTEM;

	int nDumpResult = view->DumpFileContent(file, destPath);
	return nDumpResult;
}
