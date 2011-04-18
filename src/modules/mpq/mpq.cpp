// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include <vector>

#define __STORMLIB_SELF__ 1
#include "StormLib/StormLib.h"

struct MoPaQ_File
{
	HANDLE hMpq;
	std::vector<SFILE_FIND_DATA> vFiles;
};

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	char szNameBuf[MAX_PATH] = {0};
	WideCharToMultiByte(CP_ACP, 0, params.FilePath, -1, szNameBuf, MAX_PATH, NULL, NULL);

	HANDLE hMpq = NULL;
	if (!SFileOpenArchive(szNameBuf, 0, MPQ_OPEN_READ_ONLY, &hMpq))
		return SOR_INVALID_FILE;

	SFILE_FIND_DATA ffd;
	HANDLE hSearch = SFileFindFirstFile(hMpq, "*", &ffd, NULL);
	if (hSearch == NULL)
	{
		SFileCloseArchive(hMpq);
		return SOR_INVALID_FILE;
	}

	MoPaQ_File* file = new MoPaQ_File();
	file->hMpq = hMpq;

	// Enumerate content
	do 
	{
		file->vFiles.push_back(ffd);
	} while (SFileFindNextFile(hSearch, &ffd) != 0);
	SFileFindClose(hSearch);

	*storage = file;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MoPaQ Container");
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Mixed");
	
	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (fileObj != NULL)
	{
		fileObj->vFiles.clear();
		SFileCloseArchive(fileObj->hMpq);
		delete fileObj;
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (fileObj == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= (int) fileObj->vFiles.size())
		return GET_ITEM_NOMOREITEMS;

	const SFILE_FIND_DATA &ffd = fileObj->vFiles[item_index];

	MultiByteToWideChar(CP_ACP, 0, ffd.cFileName, -1, item_path, path_size);

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	item_data->dwFileAttributes = ffd.dwFileFlags;
	item_data->nFileSizeLow = ffd.dwFileSize;
	item_data->ftLastWriteTime.dwHighDateTime = ffd.dwFileTimeHi;
	item_data->ftLastWriteTime.dwLowDateTime = ffd.dwFileTimeLo;

	wchar_t* lastSlash = wcsrchr(item_path, '\\');
	if (lastSlash && *(lastSlash + 1))
		wcscpy_s(item_data->cFileName, MAX_PATH, lastSlash + 1);
	else
		wcscpy_s(item_data->cFileName, MAX_PATH, item_path);

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
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
