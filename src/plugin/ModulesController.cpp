#include "StdAfx.h"
#include "ModulesController.h"

int ModulesController::Init( wchar_t* basePath )
{
	Cleanup();

	wstring strBasePath(basePath);
	wstring strCfgFile = strBasePath + CONFIG_FILE;

	wchar_t *wszSection = new wchar_t[1024];
	DWORD res = GetPrivateProfileSectionW(L"Modules", wszSection, 1024, strCfgFile.c_str());
	if (res == 0)
	{
		delete [] wszSection;
		return 0;
	}

	wchar_t *wszModuleDef = wszSection;
	while (wszModuleDef && *wszModuleDef)
	{
		wchar_t *wszSeparator = wcschr(wszModuleDef, '=');
		if (!wszSeparator) continue;

		// Skip commented modules
		if (wszModuleDef[0] == ';') continue;
		
		size_t nNextEntry = wcslen(wszModuleDef) + 1;
		*wszSeparator = 0;

		ExternalModule module;
		wcscpy_s(module.ModuleName, sizeof(module.ModuleName) / sizeof(module.ModuleName[0]), wszModuleDef);
		wcscpy_s(module.LibraryFile, sizeof(module.LibraryFile) / sizeof(module.LibraryFile[0]), wszSeparator + 1);

		wstring strFillModulePath = strBasePath + module.LibraryFile;
		module.ModuleHandle = LoadLibraryW(strFillModulePath.c_str());
		if (module.ModuleHandle != NULL)
		{
			module.LoadModule = (LoadSubModuleFunc) GetProcAddress(module.ModuleHandle, "LoadSubModule");
			module.OpenStorage = (OpenStorageFunc) GetProcAddress(module.ModuleHandle, "OpenStorage");
			module.CloseStorage = (CloseStorageFunc) GetProcAddress(module.ModuleHandle, "CloseStorage");
			module.GetNextItem = (GetNextItemFunc) GetProcAddress(module.ModuleHandle, "GetNextItem");
			module.Extract = (ExtractFunc) GetProcAddress(module.ModuleHandle, "ExtractItem");

			if ((module.LoadModule != NULL) && (module.OpenStorage != NULL) && (module.CloseStorage != NULL) && (module.GetNextItem != NULL))
			{
				if (module.LoadModule(0))
					modules.push_back(module);
				else
					FreeLibrary(module.ModuleHandle);
			}
			else
			{
				FreeLibrary(module.ModuleHandle);
			}
		}

		wszModuleDef += nNextEntry;
	} //while
	delete [] wszSection;
	
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
