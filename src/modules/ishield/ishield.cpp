// ishield.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"
#include "ISCabFile.h"

static const char* BOM = "\xFF\xFE";

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	ISCabFile* cabStorage = OpenCab(params.FilePath);
	if (cabStorage != NULL)
	{
		*storage = cabStorage;
		
		memset(info, 0, sizeof(StorageGeneralInfo));
		if (cabStorage->MajorVersion() != -1)
			swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Install Shield %u Cabinet", cabStorage->MajorVersion());
		else
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Install Shield SFX");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, cabStorage->GetCompression());
		
		return SOR_SUCCESS;
	}
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	ISCabFile* cabStorage = (ISCabFile*) storage;
	if (cabStorage != NULL)
	{
		CloseCab(cabStorage);
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	ISCabFile* cabStorage = (ISCabFile*) storage;
	if (cabStorage == NULL || item_index < 0) return GET_ITEM_ERROR;

	memset(item_info, 0, sizeof(StorageItemInfo));
	if (item_index == 0 && cabStorage->HasInfoData())
	{
		// First file will be always fake info file
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{info}.txt");
		item_info->Size = cabStorage->GetCabInfo().size() * sizeof(wchar_t) + strlen(BOM);
		return GET_ITEM_OK;
	}
	else if (item_index < cabStorage->GetTotalFiles())
	{
		if (cabStorage->GetFileInfo(cabStorage->HasInfoData() ? item_index - 1 : item_index, item_info))
		{
			for (size_t i = 0, len = wcslen(item_info->Path); i < len; i++)
			{
				if (item_info->Path[i] == '<')
					item_info->Path[i] = '[';
				else if (item_info->Path[i] == '>')
					item_info->Path[i] = ']';
			}
			return GET_ITEM_OK;
		}
		else
		{
			return GET_ITEM_ERROR;
		}
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	ISCabFile* cabStorage = (ISCabFile*) storage;
	if (cabStorage == NULL || params.ItemIndex < 0) return SER_ERROR_SYSTEM;

	CFileStream* pDestFile = CFileStream::Open(params.DestPath, false, true);
	if (pDestFile == nullptr) return SER_ERROR_WRITE;

	int status = SER_SUCCESS;
	if (params.ItemIndex == 0 && cabStorage->HasInfoData())
	{
		// Dump info file
		const std::wstring infoData = cabStorage->GetCabInfo();
		pDestFile->WriteBuffer(BOM, strlen(BOM));
		pDestFile->WriteBuffer(infoData.c_str(), (infoData.size() * sizeof(wchar_t)));
	}
	else if (params.ItemIndex < cabStorage->GetTotalFiles())
	{
		int itemIndex = cabStorage->HasInfoData() ? params.ItemIndex - 1 : params.ItemIndex;
		
		// Dump archive file
		status = cabStorage->ExtractFile(itemIndex, pDestFile, params.Callbacks);

		if (status == SER_SUCCESS)
		{
			StorageItemInfo itemInfo = {0};
			cabStorage->GetFileInfo(itemIndex, &itemInfo);
			
			SetFileTime(pDestFile->GetHandle(), NULL, NULL, &itemInfo.ModificationTime);
		}
	}

	delete pDestFile;
	if (status != SER_SUCCESS)
	{
		DeleteFile(params.DestPath);
	}
	return status;
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
