#ifndef _ModulesController_h__
#define _ModulesController_h__

#include "ModuleDef.h"

struct ExternalModule
{
	ExternalModule(const wchar_t* Name, const wchar_t* Library)
		: m_sModuleName(Name), m_sLibraryFile(Library), ModuleHandle(0), ModuleVersion(0)
	{
		LoadModule = NULL;
		UnloadModule = NULL;
		OpenStorage = NULL;
		CloseStorage = NULL;
		GetNextItem = NULL;
		Extract = NULL;
	}
	
	HMODULE ModuleHandle;
	std::vector<wstring> ExtensionFilter;

	LoadSubModuleFunc LoadModule;
	UnloadSubModuleFunc UnloadModule;

	DWORD ModuleVersion;
	OpenStorageFunc OpenStorage;
	CloseStorageFunc CloseStorage;
	GetItemFunc GetNextItem;
	ExtractFunc Extract;

	const wchar_t* Name() const { return m_sModuleName.c_str(); }
	const wchar_t* LibraryFile() const { return m_sLibraryFile.c_str(); }

private:
	std::wstring m_sModuleName;
	std::wstring m_sLibraryFile;
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
	vector<ExternalModule> modules;
	
	bool LoadModule(const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings);

public:
	ModulesController(void) {};
	~ModulesController(void) { this->Cleanup(); };

	int Init(const wchar_t* basePath, const wchar_t* configPath);
	void Cleanup();
	
	size_t NumModules() { return modules.size(); }
	const ExternalModule& GetModule(int index) { return modules[index]; }

	int OpenStorageFile(OpenStorageFileInParams srcParams, int *moduleIndex, HANDLE *storage, StorageGeneralInfo *info);
	void CloseStorageFile(int moduleIndex, HANDLE storage);
};

#endif // _ModulesController_h__