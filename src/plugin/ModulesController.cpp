#include "StdAfx.h"
#include "ModulesController.h"
#include "OptionsParser.h"

#define SECTION_BUF_SIZE 1024

int ModulesController::Init( wchar_t* basePath )
{
	Cleanup();

	wstring strBasePath(basePath);
	wstring strCfgFile = strBasePath + CONFIG_FILE;

	OptionsList mModulesList;

	// Get list of modules from config file
	if (!ParseOptions(strCfgFile.c_str(), L"Modules", mModulesList))
		return 0;

	wchar_t wszModuleSection[SECTION_BUF_SIZE] = {0};

	OptionsList::iterator cit;
	for (cit = mModulesList.begin(); cit != mModulesList.end(); cit++)
	{
		ExternalModule module;
		wcscpy_s(module.ModuleName, sizeof(module.ModuleName) / sizeof(module.ModuleName[0]), cit->key.c_str());
		wcscpy_s(module.LibraryFile, sizeof(module.LibraryFile) / sizeof(module.LibraryFile[0]), cit->value.c_str());

		// Get module specific settings section
		DWORD readRes = GetPrivateProfileSectionW(module.ModuleName, wszModuleSection, SECTION_BUF_SIZE, strCfgFile.c_str());
		const wchar_t* wszModuleSettings = (readRes > 0) && (readRes < SECTION_BUF_SIZE - 2) ? wszModuleSection : NULL;

		if (LoadModule(basePath, module, wszModuleSettings))
			modules.push_back(module);
	} // for
	
	return modules.size();
}

void ModulesController::Cleanup()
{
	for (size_t i = 0; i < modules.size(); i++)
		FreeLibrary(modules[i].ModuleHandle);
	modules.clear();
}

int ModulesController::OpenStorageFile(const wchar_t* path, int *moduleIndex, INT_PTR **storage, StorageGeneralInfo *sinfo)
{
	*moduleIndex = -1;
	for (size_t i = 0; i < modules.size(); i++)
		if (modules[i].OpenStorage(path, storage, sinfo) == TRUE)
		{
			*moduleIndex = i;
			return TRUE;
		}

	return FALSE;
}

void ModulesController::CloseStorageFile(int moduleIndex, INT_PTR *storage)
{
	if ((moduleIndex >= 0) && (moduleIndex < (int) modules.size()))
		modules[moduleIndex].CloseStorage(storage);
}

bool ModulesController::LoadModule( const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings )
{
	if (!module.ModuleName[0] || !module.LibraryFile[0])
		return false;

	wstring strFillModulePath(basePath);
	strFillModulePath.append(module.LibraryFile);

	module.ModuleHandle = LoadLibraryW(strFillModulePath.c_str());
	if (module.ModuleHandle != NULL)
	{
		module.LoadModule = (LoadSubModuleFunc) GetProcAddress(module.ModuleHandle, "LoadSubModule");
		module.OpenStorage = (OpenStorageFunc) GetProcAddress(module.ModuleHandle, "OpenStorage");
		module.CloseStorage = (CloseStorageFunc) GetProcAddress(module.ModuleHandle, "CloseStorage");
		module.GetNextItem = (GetItemFunc) GetProcAddress(module.ModuleHandle, "GetStorageItem");
		module.Extract = (ExtractFunc) GetProcAddress(module.ModuleHandle, "ExtractItem");

		if ((module.LoadModule != NULL) && (module.OpenStorage != NULL) &&
			(module.CloseStorage != NULL) && (module.GetNextItem != NULL) && (module.Extract != NULL))
		{
			// Try to init module
			if (module.LoadModule(moduleSettings))
				return true;
		}

		FreeLibrary(module.ModuleHandle);
		return false;
	}
	
	return false;
}
