#include "stdafx.h"
#include "wcx_loader.h"

using namespace std;

static const wchar_t* GetFileName(const wchar_t* filePath)
{
	const wchar_t* ext = wcsrchr(filePath, '\\');
	return (ext != NULL) ? ext+1 : filePath;
};


int WcxLoader::LoadModulesInDirectory( const wchar_t* basePath, bool recursive )
{
	int nFoundModules = 0;
	wstring strSearchPath(basePath);
	strSearchPath += L"*.*";

	WIN32_FIND_DATA fd = {0};
	HANDLE hSearch = FindFirstFile(strSearchPath.c_str(), &fd);
	if (hSearch == INVALID_HANDLE_VALUE) return -1;

	do 
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			if (PathMatchSpec(fd.cFileName, L"*.wcx"))
			{
				wstring modPath(basePath);
				modPath += fd.cFileName;

				IWcxModule* nextModule = LoadSingleModule(modPath.c_str());
				if (nextModule != NULL)
				{
					Modules.push_back(nextModule);
					nFoundModules++;
				}
			}
		}
		else if (recursive && (wcscmp(fd.cFileName, L"..") != 0) && (wcscmp(fd.cFileName, L".") != 0))
		{
			wstring dirPath(basePath);
			dirPath += fd.cFileName;
			dirPath += L"\\";
			
			nFoundModules += LoadModulesInDirectory(dirPath.c_str(), recursive);
		}

	} while (FindNextFile(hSearch, &fd) != 0);

	FindClose(hSearch);

	return nFoundModules;
}

int WcxLoader::LoadModules( const wchar_t* basePath, bool recursive )
{
	UnloadModules();

	//TODO: prep path
	return LoadModulesInDirectory(basePath, recursive);
}

void WcxLoader::UnloadModules()
{
	for (auto it = Modules.begin(); it != Modules.end(); ++it)
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
