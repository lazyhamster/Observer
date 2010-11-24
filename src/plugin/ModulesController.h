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
	OpenStorageFunc OpenStorage;
	CloseStorageFunc CloseStorage;
	GetItemFunc GetNextItem;
	ExtractFunc Extract;
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

	int Init(const wchar_t* basePath);
	void Cleanup();
	size_t NumModules() { return modules.size(); };

	bool OpenStorageFile(const wchar_t* path, bool applyExtFilters, int *moduleIndex, INT_PTR **storage, StorageGeneralInfo *info);
	void CloseStorageFile(int moduleIndex, INT_PTR *storage);
};

#endif // _ModulesController_h__