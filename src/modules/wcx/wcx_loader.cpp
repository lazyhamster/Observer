#include "stdafx.h"
#include "wcx_loader.h"

using namespace std;

static const wchar_t* GetFileName(const wchar_t* filePath)
{
	const wchar_t* ext = wcsrchr(filePath, '\\');
	return (ext != NULL) ? ext : filePath;
};

int WcxLoader::LoadModules( const wchar_t* basePath )
{
	UnloadModules();

	wstring strSearchPath(basePath);
	strSearchPath += L"*.wcx";

	WIN32_FIND_DATA fd = {0};
	HANDLE hSearch = FindFirstFile(strSearchPath.c_str(), &fd);
	if (hSearch == INVALID_HANDLE_VALUE) return -1;

	do 
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			wstring modPath(strSearchPath);
			modPath += fd.cFileName;

			IWcxModule* nextModule = LoadSingleModule(modPath.c_str());
			if (nextModule != NULL)
			{
				Modules.push_back(nextModule);
			}
		}

	} while (FindNextFile(hSearch, &fd) != 0);

	FindClose(hSearch);
	
	return (int) Modules.size();
}

void WcxLoader::UnloadModules()
{
	vector<IWcxModule*>::iterator it;
	for (it = Modules.begin(); it != Modules.end(); it++)
	{
		IWcxModule* module = *it;
		module->Unload();
		
		delete module;
	}

	Modules.clear();
}

#define TRY_LOAD(modClass, modHandle, modName, cp) { modClass *modVar = new modClass(); \
	if (modVar->Load(modHandle, modName, cp)) return modVar; else delete modVar; }

IWcxModule* WcxLoader::LoadSingleModule( const wchar_t* path )
{
	int defaultCP = GetACP();
	const wchar_t* moduleName = GetFileName(path);
	
	HMODULE hMod = LoadLibrary(path);
	if (hMod == NULL) return NULL;

	TRY_LOAD(WcxUnicodeModule, hMod, moduleName, defaultCP);
	TRY_LOAD(WcxAnsiExtModule, hMod, moduleName, defaultCP);
	TRY_LOAD(WcxAnsiModule, hMod, moduleName, defaultCP);
		
	FreeLibrary(hMod);
	return NULL;
}
