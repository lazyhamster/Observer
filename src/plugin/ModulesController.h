#ifndef _ModulesController_h__
#define _ModulesController_h__

#include "ModuleDef.h"

struct ExternalModule
{
	wchar_t ModuleName[32];
	wchar_t LibraryFile[32];
	HMODULE ModuleHandle;
	wchar_t ExtensionFilter[128];

	LoadSubModuleFunc LoadModule;
	UnloadSubModuleFunc UnloadModule;

	DWORD ModuleVersion;
	OpenStorageFunc OpenStorage;
	CloseStorageFunc CloseStorage;
	GetItemFunc GetNextItem;
	ExtractFunc Extract;
};

struct OpenStorageFileInParams
{
	const wchar_t* path;
	const char* password;
	bool applyExtFilters;
	int openWithModule;  // set -1 to poll all modules
};

class ModulesController
{
private:
	bool LoadModule(const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings);
	bool DoesExtensionFilterMatch(const wchar_t* path, const wchar_t* filter);

public:
	vector<ExternalModule> modules;
	
	ModulesController(void) {};
	~ModulesController(void) { this->Cleanup(); };

	int Init(const wchar_t* basePath, const wchar_t* configPath);
	void Cleanup();
	size_t NumModules() { return modules.size(); };

	int OpenStorageFile(OpenStorageFileInParams srcParams, int *moduleIndex, HANDLE *storage, StorageGeneralInfo *info);
	void CloseStorageFile(int moduleIndex, HANDLE storage);
};

#endif // _ModulesController_h__