// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "vp_file.h"

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	// Check for file extension
	size_t nPathLen = wcslen(path);
	if ( (nPathLen < 4) || (_wcsicmp(path + nPathLen - 3, L".vp") != 0) )
		return FALSE;

	// Open container
	CVPFile* file = new CVPFile();
	if (file->Open(path))
	{
		*storage = (INT_PTR*) file;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Volition Pack");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"Version 2");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

		return TRUE;
	}
	
	delete file;
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	if (storage)
	{
		CVPFile *file = (CVPFile*) storage;
		delete file;
	}
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CVPFile* file = (CVPFile*) storage;
	if (!file) return GET_ITEM_ERROR;

	if ((item_index < 0) || (item_index >= (int) file->ItemCount()))
		return GET_ITEM_NOMOREITEMS;

	VP_FileRec frec;
	if (!file->GetItem(item_index, frec))
		return GET_ITEM_ERROR;

	wcscpy_s(item_path, path_size, frec.full_path);

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, GetFileName(frec.full_path));
	wcscpy_s(item_data->cAlternateFileName, 14, L"");
	item_data->dwFileAttributes = (frec.IsDir()) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	item_data->nFileSizeLow = frec.size;
	item_data->ftLastWriteTime = frec.timestamp;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	CVPFile* file = (CVPFile*) storage;
	if (!file) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= (int) file->ItemCount())
		return SER_ERROR_SYSTEM;

	return file->ExtractSingleFile(params.item, params.destFilePath, &(params.callbacks));
}
