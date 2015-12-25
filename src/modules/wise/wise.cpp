// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include "WiseFile.h"
#include "WiseSfxFile.h"

CBaseWiseFile* TryOpenStorage(const wchar_t* path)
{
	CBaseWiseFile* arc = new CWiseFile();
	if (arc->Open(path))
	{
		return arc;
	}
	delete arc;

	arc = new CWiseSfxFile();
	if (arc->Open(path))
	{
		return arc;
	}
	delete arc;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	if (!SignatureMatchOrNull(params.Data, params.DataSize, FILE_SIGNATURE_EXE))
		return SOR_INVALID_FILE;
	
	CBaseWiseFile* arc = TryOpenStorage(params.FilePath);
	if (!arc)
	{
		return SOR_INVALID_FILE;
	}

	*storage = arc;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, arc->GetFormatName());
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, arc->GetCompression());
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");

	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	CBaseWiseFile* arc = (CBaseWiseFile *) storage;
	if (arc) delete arc;
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CBaseWiseFile* arc = (CBaseWiseFile *) storage;
	if (!arc || (item_index < 0)) return GET_ITEM_ERROR;

	WiseFileRec fileData = {0};
	bool noMoreItems = false;
	if (arc->GetFileInfo(item_index, &fileData, noMoreItems))
	{
		memset(item_info, 0, sizeof(StorageItemInfo));
		item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
		item_info->Size = fileData.UnpackedSize;
		item_info->PackedSize = fileData.PackedSize;

		if (fileData.FileName[0])
			wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), fileData.FileName);
		else
			swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"file%04d.bin", item_index);

		return GET_ITEM_OK;
	}

	return (noMoreItems) ? GET_ITEM_NOMOREITEMS : GET_ITEM_ERROR;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CBaseWiseFile* arc = (CBaseWiseFile *) storage;
	if (!arc) return SER_ERROR_SYSTEM;

	if (params.ItemIndex < 0 || params.ItemIndex >= arc->NumFiles())
		return SER_ERROR_SYSTEM;

	bool rval = arc->ExtractFile(params.ItemIndex, params.DestPath);
	return rval ? SER_SUCCESS : SER_ERROR_WRITE;
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

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
