#ifndef _ModulesController_h__
#define _ModulesController_h__

#include "ModuleDef.h"
#include "Config.h"

struct ExternalModule
{
	ExternalModule(const wchar_t* Name, const wchar_t* Library)
		: m_sModuleName(Name), m_sLibraryFile(Library)
	{
		LoadModule = nullptr;
		UnloadModule = nullptr;
		ModuleHandle = 0;
		ModuleVersion = 0;
		ModuleId = GUID_NULL;
		memset(&ModuleFunctions, 0, sizeof(ModuleFunctions));
		ShortCut = '\0';
	}
	
	HMODULE ModuleHandle;
	std::vector<std::wstring> ExtensionFilter;

	LoadSubModuleFunc LoadModule;
	UnloadSubModuleFunc UnloadModule;

	GUID ModuleId;
	DWORD ModuleVersion;
	module_cbs ModuleFunctions;

	wchar_t ShortCut;

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
	const void* dataBuffer;
	size_t dataSize;
};

struct FailedModuleInfo 
{
	wstring ModuleName;
	wstring ErrorMessage;
};

class ModulesController
{
private:
	std::vector<ExternalModule> modules;
	
	bool LoadModule(const wchar_t* basePath, ExternalModule &module, const wchar_t* moduleSettings, std::wstring &errorMsg);
	bool IsValidModuleLoaded(const ModuleLoadParameters &params);
	bool GetExceptionMessage(unsigned long exceptionCode, std::wstring &errorText);
	bool GetExceptionMessage(unsigned long exceptionCode, wchar_t* errTextBuf, size_t errTextBufSize);

public:
	ModulesController(void) {};
	~ModulesController(void) { this->Cleanup(); };

	int Init(const wchar_t* basePath, Config* cfg, std::vector<FailedModuleInfo> &failed);
	void Cleanup();
	
	size_t NumModules() { return modules.size(); }
	const ExternalModule& GetModule(int index) { return modules[index]; }

	int OpenStorageFile(OpenStorageFileInParams srcParams, int *moduleIndex, HANDLE *storage, StorageGeneralInfo *info);
	void CloseStorageFile(int moduleIndex, HANDLE storage);
};

#endif // _ModulesController_h__