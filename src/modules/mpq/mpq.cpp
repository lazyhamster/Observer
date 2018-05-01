// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "modulecrt/OptionsParser.h"
#include <vector>

#define __STORMLIB_SELF__ 1
#include "StormLib/src/StormLib.h"

static char optListfilesLocation[MAX_PATH] = { 0 };
static char optListfilesMask[32] = "*.*";
static bool optListfilesRecursive = true;

static void TrimStr(char* str)
{
	size_t strLen = strlen(str);

	// Trim at the end
	while (strLen > 0 && (str[strLen - 1] == ' ' || str[strLen - 1] == '\n' || str[strLen - 1] == '\r'))
	{
		str[strLen - 1] = 0;
		strLen--;
	}

	// Trim at the start
	while (strLen > 0 && (str[0] == ' ' || str[0] == '\n' || str[0] == '\r'))
	{
		memmove(str, str + 1, strLen - 1);
		strLen--;
	}
}

void IncludeTrailingPathDelim(char *pathBuf, size_t bufMaxSize)
{
	size_t nPathLen = strlen(pathBuf);
	if (pathBuf[nPathLen - 1] != '\\')
		strcat_s(pathBuf, bufMaxSize, "\\");
}

struct MoPaQ_File
{
	HANDLE hMpq;
	bool isEncrypted;
	std::vector<SFILE_FIND_DATA> vFiles;
};

static inline UINT get_lcid_codepage( LCID lcid )
{
	UINT ret;
	if (!GetLocaleInfo( lcid, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER, (WCHAR *)&ret, sizeof(ret)/sizeof(WCHAR) ))
		ret = 0;
	return ret;
}

static void AddListfilesFromDirectory(HANDLE hMpq, const char *sDir, bool bRecursive = true)
{
	char sPath[MAX_PATH];

	strcpy_s(sPath, sizeof(sPath) / sizeof(sPath[0]), sDir);
	strcat_s(sPath, sizeof(sPath) / sizeof(sPath[0]), optListfilesMask);

	WIN32_FIND_DATAA fdFile;
	HANDLE hFind = FindFirstFileA(sPath, &fdFile);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(fdFile.cFileName, ".") == 0 ||
			strcmp(fdFile.cFileName, "..") == 0)
			continue;

		strcpy_s(sPath, sizeof(sPath) / sizeof(sPath[0]), sDir);
		strcat_s(sPath, sizeof(sPath) / sizeof(sPath[0]), fdFile.cFileName);

		if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			SFileAddListFile(hMpq, sPath);
		}
		else if (bRecursive)
		{
			strcat_s(sPath, sizeof(sPath) / sizeof(sPath[0]), "\\");
			AddListfilesFromDirectory(hMpq, sPath);
		}
	} while (FindNextFileA(hFind, &fdFile));

	FindClose(hFind);
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	HANDLE hMpq = NULL;
	bool fEncrypted = false;

	// First try to open file as plain MPQ
	if (!SFileOpenArchive(params.FilePath, 0, STREAM_PROVIDER_FLAT | MPQ_OPEN_READ_ONLY, &hMpq))
	{
		// Additionally try to open file as encrypted MPQ
		fEncrypted = true;
		if (!SFileOpenArchive(params.FilePath, 0, STREAM_PROVIDER_MPQE | MPQ_OPEN_READ_ONLY, &hMpq))
			return SOR_INVALID_FILE;
	}

	if (optListfilesLocation[0])
	{
		// TODO: Monitor the listfiles folder updates instead of scanning it every time
		AddListfilesFromDirectory(hMpq, optListfilesLocation, optListfilesRecursive);
	}

	// Try to enum file, if failed then don't claim archive
	SFILE_FIND_DATA ffd;
	HANDLE hSearch = SFileFindFirstFile(hMpq, "*", &ffd, NULL);
	if (hSearch == NULL)
	{
		SFileCloseArchive(hMpq);
		return SOR_INVALID_FILE;
	}
	SFileFindClose(hSearch);

	MoPaQ_File* file = new MoPaQ_File();
	file->hMpq = hMpq;
	file->isEncrypted = fEncrypted;

	*storage = file;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MoPaQ Container");
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, file->isEncrypted ? L"Mixed (Encrypted)" : L"Mixed");
	
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

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (fileObj == NULL) return FALSE;
	
	SFILE_FIND_DATA ffd = {0};
	HANDLE hSearch = SFileFindFirstFile(fileObj->hMpq, "*", &ffd, NULL);
	if (hSearch == NULL) return FALSE;

	do 
	{
		fileObj->vFiles.push_back(ffd);
	} while (SFileFindNextFile(hSearch, &ffd) != 0);

	SFileFindClose(hSearch);
	
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (fileObj == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= (int) fileObj->vFiles.size())
		return GET_ITEM_NOMOREITEMS;

	const SFILE_FIND_DATA &ffd = fileObj->vFiles[item_index];

	UINT itemCP = get_lcid_codepage(ffd.lcLocale);

	memset(item_info, 0, sizeof(StorageItemInfo));
	MultiByteToWideChar(itemCP, 0, ffd.cFileName, -1, item_info->Path, STRBUF_SIZE(item_info->Path));
	item_info->Size = ffd.dwFileSize;
	item_info->PackedSize = ffd.dwCompSize;
	item_info->ModificationTime.dwHighDateTime = ffd.dwFileTimeHi;
	item_info->ModificationTime.dwLowDateTime = ffd.dwFileTimeLo;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	MoPaQ_File* fileObj = (MoPaQ_File*) storage;
	if (!fileObj) return SER_ERROR_SYSTEM;

	if (params.ItemIndex < 0 || params.ItemIndex >= (int) fileObj->vFiles.size())
		return SER_ERROR_SYSTEM;

	const SFILE_FIND_DATA &ffd = fileObj->vFiles[params.ItemIndex];

	HANDLE hInFile = NULL;
	HANDLE hOutFile;

	bool fIsListfile = strcmp(ffd.cFileName, LISTFILE_NAME) == 0;
	if (!SFileOpenFileEx(fileObj->hMpq, ffd.cFileName, fIsListfile ? SFILE_OPEN_ANY_LOCALE : SFILE_OPEN_FROM_MPQ, &hInFile))
		return SER_ERROR_READ;
	
	hOutFile = CreateFileW(params.DestPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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
			params.Callbacks.FileProgress(params.Callbacks.signalContext, dwBytes);

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

// {2154CA1D-F66A-464F-A42C-41A70318EA7B}
static const GUID MODULE_GUID = { 0x2154ca1d, 0xf66a, 0x464f, { 0xa4, 0x2c, 0x41, 0xa7, 0x3, 0x18, 0xea, 0x7b } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 1);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"ListfilesLocation", optListfilesLocation, sizeof(optListfilesLocation) / sizeof(optListfilesLocation[0]));
	opts.GetValue(L"ListfilesMask", optListfilesMask, sizeof(optListfilesMask) / sizeof(optListfilesMask[0]));
	opts.GetValue(L"ListfilesRecursive", optListfilesRecursive);

	TrimStr(optListfilesLocation);
	IncludeTrailingPathDelim(optListfilesLocation, sizeof(optListfilesLocation) / sizeof(optListfilesLocation[0]));

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
