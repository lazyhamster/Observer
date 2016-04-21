// pst.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "modulecrt/OptionsParser.h"
#include "PstProcessing.h"

using namespace std;

static bool optExpandEmlFile = true;
static bool optHideEmptyFolders = false;

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	wstring strPath(params.FilePath);
	pst *storeObj = NULL;
	
	try
	{
		storeObj = new pst(strPath);
		wstring strDbName = storeObj->get_name();
		
		PstFileInfo *objInfo = new PstFileInfo(storeObj);
		objInfo->HideEmptyFolders = optHideEmptyFolders;
		objInfo->ExpandEmlFile = optExpandEmlFile;

		*storage = objInfo;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Outlook DB");
		wcsncpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, strDbName.c_str(), _TRUNCATE);
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

		return SOR_SUCCESS;

	}
	catch(...)
	{
		if (storeObj) delete storeObj;
	}
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (file)
	{
		delete file;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (!file) return FALSE;

	folder pRoot = file->PstObject->open_root_folder();
	return process_folder(pRoot, file, L"") ? TRUE : FALSE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (!file) return GET_ITEM_ERROR;

	if (item_index < 0) return GET_ITEM_ERROR;
	if (item_index >= (int) file->Entries.size())
		return GET_ITEM_NOMOREITEMS;

	const PstFileEntry &fentry = file->Entries[item_index];
	
	memset(item_info, 0, sizeof(StorageItemInfo));
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), fentry.GetFullPath().c_str());
	wcsncpy_s(item_info->Owner, STRBUF_SIZE(item_info->Owner), fentry.Sender.c_str(), STRBUF_SIZE(item_info->Owner) - 1);
	item_info->Attributes = (fentry.Type == ETYPE_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	item_info->Size = fentry.GetSize();
	item_info->ModificationTime = fentry.LastModificationTime;
	item_info->CreationTime = fentry.CreationTime;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (!file) return SER_ERROR_SYSTEM;

	if (params.ItemIndex < 0 || params.ItemIndex >= (int) file->Entries.size())
		return SER_ERROR_SYSTEM;

	HANDLE hOutFile = CreateFile(params.DestPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hOutFile == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	const PstFileEntry &fentry = file->Entries[params.ItemIndex];
	ExtractResult nWriteResult = fentry.ExtractContent(hOutFile);
	CloseHandle(hOutFile);
	
	if (nWriteResult != ER_SUCCESS)
		DeleteFile(params.DestPath);

	switch(nWriteResult)
	{
	case ER_SUCCESS:
		return SER_SUCCESS;
	case ER_ERROR_READ:
		return SER_ERROR_READ;
	case ER_ERROR_WRITE:
		return SER_ERROR_WRITE;
	}

	return SER_ERROR_SYSTEM;
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

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"HideEmptyFolders", optHideEmptyFolders);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
