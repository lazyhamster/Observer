// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ViseFile.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	CViseFile* arc = new CViseFile();
	if (!arc->Open(params.FilePath))
	{
		delete arc;
		return SOR_INVALID_FILE;
	}

	*storage = arc;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Vise Installer");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"ZLib");
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");

	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	CViseFile* arc = (CViseFile *) storage;
	if (arc) delete arc;
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	CViseFile* arc = (CViseFile *) storage;
	if (!arc || (item_index < 0)) return GET_ITEM_ERROR;

	if (item_index < (int) arc->GetFilesCount())
	{
		const ViseFileEntry& entry = arc->GetFileInfo(item_index);
		
		memset(item_info, 0, sizeof(StorageItemInfo));
		MultiByteToWideChar(CP_ACP, 0, entry.Name.c_str(), -1, item_info->Path, STRBUF_SIZE(item_info->Path));
		item_info->Size = entry.UnpackedSize;
		item_info->PackedSize = entry.PackedSize;
		item_info->ModificationTime = entry.LastWriteTime;
		
		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	CViseFile* arc = (CViseFile *) storage;
	if (!arc || (params.ItemIndex < 0) || (params.ItemIndex >= (int) arc->GetFilesCount())) return SER_ERROR_SYSTEM;
	
	CFileStream* outStream = CFileStream::Open(params.DestPath, false, true);
	if (!outStream)	return SER_ERROR_WRITE;

	bool extractResult = arc->ExtractFile(params.ItemIndex, outStream);
	delete outStream;

	if (!extractResult)
	{
		DeleteFile(params.DestPath);
		return SER_ERROR_READ;
	}

	return SER_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {D12F958B-BB55-4DB3-AA56-3A869D95D5C1}
static const GUID MODULE_GUID = { 0xd12f958b, 0xbb55, 0x4db3, { 0xaa, 0x56, 0x3a, 0x86, 0x9d, 0x95, 0xd5, 0xc1 } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
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
