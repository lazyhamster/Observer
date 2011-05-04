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

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (fileObj == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= (int) fileObj->vFiles.size())
		return GET_ITEM_NOMOREITEMS;

	const SFILE_FIND_DATA &ffd = fileObj->vFiles[item_index];

	memset(item_info, 0, sizeof(StorageItemInfo));
	MultiByteToWideChar(CP_ACP, 0, ffd.cFileName, -1, item_info->Path, STRBUF_SIZE(item_info->Path));
	item_info->Attributes = ffd.dwFileFlags;
	item_info->Size = ffd.dwFileSize;
	item_info->ModificationTime.dwHighDateTime = ffd.dwFileTimeHi;
	item_info->ModificationTime.dwLowDateTime = ffd.dwFileTimeLo;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (!fileObj) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= (int) fileObj->vFiles.size())
		return SER_ERROR_SYSTEM;

	const SFILE_FIND_DATA &ffd = fileObj->vFiles[params.item];

	HANDLE hInFile = NULL;
	HANDLE hOutFile;

	if (!SFileOpenFileEx(fileObj->hMpq, ffd.cFileName, 0, &hInFile))
		return SER_ERROR_READ;
	
	hOutFile = CreateFileW(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hOutFile == INVALID_HANDLE_VALUE)
	{
		SFileCloseFile(hInFile);
		return SER_ERROR_WRITE;
	}

	char copyBuf[32 * 1024];
	DWORD dwBytes = 1;
	int nRetVal = SER_SUCCESS;

	while (dwBytes > 0)
	{
		if (!SFileReadFile(hInFile, copyBuf, sizeof(copyBuf), &dwBytes, NULL) && (GetLastError() != ERROR_HANDLE_EOF))
		{
			nRetVal = SER_ERROR_READ;
			break;
		}

		if (dwBytes > 0)
		{
			params.callbacks.FileProgress(params.callbacks.signalContext, dwBytes);

			if (!WriteFile(hOutFile, copyBuf, dwBytes, &dwBytes, NULL))
			{
				nRetVal = SER_ERROR_WRITE;
				break;
			}
		}
	}

	SFileCloseFile(hInFile);
	CloseHandle(hOutFile);

	return nRetVal;
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
