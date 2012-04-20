#include "stdafx.h"
#include "wcx_loader.h"

using namespace std;

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

			WcxModule* nextModule = LoadSingleModule(modPath.c_str());
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
	vector<WcxModule*>::iterator it;
	for (it = Modules.begin(); it != Modules.end(); it++)
	{
		WcxModule* module = *it;
		FreeLibrary(module->ModuleHandle);
	}

	Modules.clear();
}

WcxModule* WcxLoader::LoadSingleModule( const wchar_t* path )
{
	HMODULE hMod = LoadLibrary(path);
	if (hMod == NULL) return NULL;

	WcxModule* mod = new WcxModule();

	mod->CanYouHandleThisFileW = (CanYouHandleThisFileWFunc) GetProcAddress(hMod, "CanYouHandleThisFileW");
	mod->OpenArchiveW = (OpenArchiveWFunc) GetProcAddress(hMod, "OpenArchiveW");
	mod->ProcessFileW = (ProcessFileWFunc) GetProcAddress(hMod, "ProcessFileW");
	mod->ReadHeaderExW = (ReadHeaderExWFunc) GetProcAddress(hMod, "ReadHeaderExW");
	mod->SetProcessDataProcW = (SetProcessDataProcWFunc) GetProcAddress(hMod, "SetProcessDataProcW");

	mod->CanYouHandleThisFile = (CanYouHandleThisFileFunc) GetProcAddress(hMod, "CanYouHandleThisFile");
	mod->OpenArchive = (OpenArchiveFunc) GetProcAddress(hMod, "OpenArchive");
	mod->ProcessFile = (ProcessFileFunc) GetProcAddress(hMod, "ProcessFile");
	mod->ReadHeader = (ReadHeaderFunc) GetProcAddress(hMod, "ReadHeader");
	mod->ReadHeaderEx = (ReadHeaderExFunc) GetProcAddress(hMod, "ReadHeaderEx");
	mod->SetProcessDataProc = (SetProcessDataProcFunc) GetProcAddress(hMod, "SetProcessDataProc");

	mod->CloseArchive = (CloseArchiveFunc) GetProcAddress(hMod, "CloseArchive");

	// Calculate module type
	WcxModuleType type = WCMT_INVALID;
	
	if ((mod->CanYouHandleThisFileW != NULL) && (mod->OpenArchiveW != NULL) && (mod->ReadHeaderExW != NULL)
		&& (mod->ProcessFileW != NULL) && (mod->SetProcessDataProcW != NULL) && (mod->CloseArchive != NULL))
	{
		type = WCMT_UNICODE;
	}
	else if ((mod->CanYouHandleThisFile != NULL) && (mod->OpenArchive != NULL) && (mod->ProcessFile != NULL)
		&& (mod->SetProcessDataProc != NULL) && (mod->CloseArchive != NULL))
	{
		if (mod->ReadHeaderEx != NULL)
			type = WCMT_ANSIEX;
		else if (mod->ReadHeader != NULL)
			type = WCMT_ANSI;
	}

	if (type != WCMT_INVALID)
	{
		mod->ModuleHandle = hMod;
		mod->Type = type;

		return mod;
	}
	
	FreeLibrary(hMod);
	delete mod;

	return NULL;
}
