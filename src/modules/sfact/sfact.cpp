// sfact.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "SfFile.h"
#include "ModuleCRT.h"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	if (!SignatureMatchOrNull(params.Data, params.DataSize, FILE_SIGNATURE_EXE))
		return SOR_INVALID_FILE;
	
	SetupFactoryFile* sfInst = OpenInstaller(params.FilePath);
	if ((sfInst != nullptr) && (sfInst->EnumFiles() > 0))
	{
		*storage = sfInst;

		memset(info, 0, sizeof(StorageGeneralInfo));
		swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Setup Factory %d", sfInst->GetVersion());
		switch(sfInst->GetCompression())
		{
			case COMP_NONE:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");
				break;
			case COMP_PKWARE:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"PKWare DCL");
				break;
			case COMP_LZMA:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"LZMA");
				break;
			case COMP_LZMA2:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"LZMA2");
				break;
			default:
				wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Unknown");
				break;
		}
				
		return SOR_SUCCESS;
	}

	if (sfInst != nullptr)
	{
		FreeInstaller(sfInst);
	}

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst != NULL)
	{
		FreeInstaller(sfInst);
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index < (int) sfInst->GetCount())
	{
		SFFileEntry fe = sfInst->GetFile(item_index);
		
		memset(item_info, 0, sizeof(StorageItemInfo));
		MultiByteToWideChar(sfInst->GetFileNameEncoding(), 0, fe.LocalPath.c_str(), -1, item_info->Path, _countof(item_info->Path));
		item_info->Size = fe.UnpackedSize;
		item_info->PackedSize = fe.PackedSize;
		item_info->Attributes = fe.Attributes;
		if (fe.LastWriteTime != 0) UnixTimeToFileTime(fe.LastWriteTime, &item_info->ModificationTime);
		if (fe.CreationTime != 0) UnixTimeToFileTime(fe.CreationTime, &item_info->CreationTime);
		
		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	SetupFactoryFile* sfInst = (SetupFactoryFile*) storage;
	if (sfInst == NULL || params.ItemIndex < 0 || params.ItemIndex >= (int) sfInst->GetCount()) return SER_ERROR_SYSTEM;

	CFileStream* outStream = CFileStream::Open(params.DestPath, false, true);
	if (!outStream)	return SER_ERROR_WRITE;
	
	bool extractResult = sfInst->ExtractFile(params.ItemIndex, outStream);
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

// {A9EF2D5E-35D1-47D3-8FD4-54C0708F39CA}
static const GUID MODULE_GUID = { 0xa9ef2d5e, 0x35d1, 0x47d3, { 0x8f, 0xd4, 0x54, 0xc0, 0x70, 0x8f, 0x39, 0xca } };

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
