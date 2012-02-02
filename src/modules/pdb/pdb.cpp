// pdb.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

struct PdbFileInfo 
{
	IPdbFile* pdb;
	std::vector<IPdbModule*> pdbModules;
};

static IPdbParser* g_ParserPtr = NULL;

static void RenameDiskNameColons(wchar_t* path)
{
	wchar_t* ptr = path;
	while (*ptr != '\0')
	{
		if (*ptr == ':') *ptr = '_';
		ptr++;
	}
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	IPdbFile* pdbFile = g_ParserPtr->OpenFile(params.FilePath);
	
	if (pdbFile != NULL)
	{
		PdbFileInfo* file = new PdbFileInfo();
		file->pdb = pdbFile;
		file->pdbModules = pdbFile->GetModules();
		
		*storage = file;

		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Program Database");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, pdbFile->IsStripped() ? L"Stripped" : L"-");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

		return SOR_SUCCESS;
	}
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	PdbFileInfo* pdbInfo = (PdbFileInfo*)storage;
	if (pdbInfo == NULL) return;

	pdbInfo->pdb->Close();
	delete pdbInfo;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	PdbFileInfo* pdbInfo = (PdbFileInfo*)storage;
	if (pdbInfo == NULL || item_index < 0) return GET_ITEM_ERROR;
	
	if (item_index == 0)
	{
		// Info file
		memset(item_info, 0, sizeof(StorageItemInfo));
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"info.txt");
		item_info->Size = 10;

		return GET_ITEM_OK;
	}
	else if (item_index-1 < (int) pdbInfo->pdbModules.size())
	{
		// Module file
		const IPdbModule* pdbModule = pdbInfo->pdbModules[item_index - 1];

		memset(item_info, 0, sizeof(StorageItemInfo));
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{modules}\\%s", pdbModule->GetName().c_str());
		RenameDiskNameColons(item_info->Path);
		item_info->Size = 20;

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	PdbFileInfo* pdbInfo = (PdbFileInfo*)storage;
	if (pdbInfo == NULL || params.item < 0) return GET_ITEM_ERROR;

	if (params.item == 0)
	{
		// Info file
	}
	else if (params.item-1 < (int) pdbInfo->pdbModules.size())
	{
		// Module file
	}
	
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	try
	{
		g_ParserPtr = IPdbParserFactory::Create();
		if (g_ParserPtr == NULL) return FALSE;
	}
	catch (...)
	{
		return FALSE;
	}
	
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
	IPdbParserFactory::Destroy();
}
