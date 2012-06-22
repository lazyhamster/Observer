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
		FreeLibrary(module->m_ModuleHandle);
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
	
	if ((mod->OpenArchiveW != NULL) && (mod->ReadHeaderExW != NULL)	&& (mod->ProcessFileW != NULL) && (mod->SetProcessDataProcW != NULL) && (mod->CloseArchive != NULL))
	{
		type = WCMT_UNICODE;
	}
	else if ((mod->OpenArchive != NULL) && (mod->ProcessFile != NULL) && (mod->SetProcessDataProc != NULL) && (mod->CloseArchive != NULL))
	{
		if (mod->ReadHeaderEx != NULL)
			type = WCMT_ANSIEX;
		else if (mod->ReadHeader != NULL)
			type = WCMT_ANSI;
	}

	if (type != WCMT_INVALID)
	{
		mod->m_ModuleHandle = hMod;
		mod->m_ModuleName = GetFileName(path);
		mod->Type = type;

		return mod;
	}
	
	FreeLibrary(hMod);
	delete mod;

	return NULL;
}

//////////////////////////////////////////////////////////////////////////

bool WcxModule::WcxIsArchive( const wchar_t* wszFilePath )
{
	if (CanYouHandleThisFileW != NULL)
	{
		return CanYouHandleThisFileW(wszFilePath) != FALSE;
	}

	if (CanYouHandleThisFile != NULL)
	{
		char fileName[MAX_PATH] = {0};
		WideCharToMultiByte(CP_ACP, 0, wszFilePath, -1, fileName, MAX_PATH, NULL, NULL);
		return CanYouHandleThisFile(fileName) != FALSE;
	}
	
	return true;
}

HANDLE WcxModule::WcxOpenArchive( const wchar_t* wszFilePath, int nOpMode )
{
	switch (Type)
	{
		case WCMT_UNICODE:
			{
				tOpenArchiveDataW oad = {0};
				oad.ArcName = const_cast<wchar_t*>(wszFilePath);
				oad.OpenMode = nOpMode;

				return OpenArchiveW(&oad);
			}
		case WCMT_ANSI:
		case WCMT_ANSIEX:
			{
				char fileNameBuf[MAX_PATH] = {0};
				WideCharToMultiByte(CP_ACP, 0, wszFilePath, -1, fileNameBuf, MAX_PATH, NULL, NULL);
				
				tOpenArchiveData oad = {0};
				oad.ArcName = fileNameBuf;
				oad.OpenMode = nOpMode;
				return OpenArchive(&oad);
			}
		default:
			return NULL;
	}
}

void WcxModule::WcsCloseArchive( HANDLE hArchive )
{
	if (hArchive != NULL)
	{
		CloseArchive(hArchive);
	}
}

int WcxModule::WcxReadHeader( HANDLE hArchive, tHeaderDataExW *HeaderData )
{
	return 0;
}

int WcxModule::WcxProcessFile( HANDLE hArchive, int Operation, wchar_t *DestPath )
{
	return 0;
}
