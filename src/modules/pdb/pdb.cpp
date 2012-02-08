// pdb.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

static IPdbParser* g_ParserPtr = NULL;

size_t GenerateInfoFileContent(const PdbFileInfo* pdbInfo, std::string &buf);
size_t GenerateModuleFileContent(IPdbModule* module, std::string &buf);

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
	
	std::string buf;
	if (item_index == 0)
	{
		// Info file
		memset(item_info, 0, sizeof(StorageItemInfo));
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"info.txt");
		item_info->Size = GenerateInfoFileContent(pdbInfo, buf);

		return GET_ITEM_OK;
	}
	else if (item_index-1 < (int) pdbInfo->pdbModules.size())
	{
		// Module file
		IPdbModule* pdbModule = pdbInfo->pdbModules[item_index - 1];
		std::wstring moduleName = pdbModule->GetName();
		
		// Convert disk name to uppercase
		if (moduleName[1] == ':')
			moduleName[0] = toupper(moduleName[0]);

		memset(item_info, 0, sizeof(StorageItemInfo));
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{modules}\\%s", moduleName.c_str());
		RenameDiskNameColons(item_info->Path);
		item_info->Size = GenerateModuleFileContent(pdbModule, buf);

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	PdbFileInfo* pdbInfo = (PdbFileInfo*)storage;
	if (pdbInfo == NULL || params.item < 0) return GET_ITEM_ERROR;

	std::string buf;
	if (params.item == 0)
	{
		// Info file
		GenerateInfoFileContent(pdbInfo, buf);
	}
	else if (params.item-1 < (int) pdbInfo->pdbModules.size())
	{
		// Module file
		IPdbModule* pdbModule = pdbInfo->pdbModules[params.item - 1];
		GenerateModuleFileContent(pdbModule, buf);
	}
	else
	{
		return SER_ERROR_SYSTEM;
	}

	HANDLE hOut = CreateFile(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hOut == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	DWORD nWritten;
	WriteFile(hOut, buf.c_str(), (DWORD) buf.length(), &nWritten, NULL);
	CloseHandle(hOut);

	return SER_SUCCESS;
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
