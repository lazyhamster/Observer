// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "WiseFile.h"


int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	CWiseFile* arc = new CWiseFile();
	if (!arc->Open(path))
	{
		delete arc;
		return FALSE;
	}

	*storage = (INT_PTR *) arc;

	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Wise");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"zlib");
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	memset(&info->Created, 0, sizeof(info->Created));

	return TRUE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	CWiseFile* arc = (CWiseFile *) storage;
	if (arc) delete arc;
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CWiseFile* arc = (CWiseFile *) storage;
	if (!arc || (item_index < 0)) return GET_ITEM_ERROR;

	WiseFileRec fileData = {0};
	bool noMoreItems = false;
	if (arc->GetFileInfo(item_index, &fileData, noMoreItems))
	{
		memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
		
		wchar_t* wszSlash = wcsrchr(fileData.FileName, '\\');
		if (wszSlash)
			wcscpy_s(item_data->cFileName, MAX_PATH, wszSlash + 1);
		else
			wcscpy_s(item_data->cFileName, MAX_PATH, fileData.FileName);
		item_data->nFileSizeLow = fileData.UnpackedSize;

		wcscpy_s(item_path, path_size, fileData.FileName);
		return GET_ITEM_OK;
	}

	return (noMoreItems) ? GET_ITEM_NOMOREITEMS : GET_ITEM_ERROR;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	CWiseFile* arc = (CWiseFile *) storage;
	if (!arc) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= arc->NumFiles())
		return SER_ERROR_READ;
	
	return SER_ERROR_SYSTEM;
}
