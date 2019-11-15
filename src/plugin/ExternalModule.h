#ifndef ExternalModule_h__
#define ExternalModule_h__

#include "ModuleDef.h"
#include "ExtFilter.h"

enum class ModuleLoadStatus
{
	Success,
	LoadLibraryFailed,
	NotModule,
	InvalidAPIVersion,
	ExceptionCaught,
	LoadModuleFailed
};

struct ModuleLoadResult
{
	ModuleLoadStatus Status;
	unsigned long ErrorCode;
};

struct ExternalModule
{
	module_cbs ModuleFunctions;

	wchar_t ShortCut;

	ExternalModule(const std::wstring& Name, const std::wstring& Library);
	~ExternalModule();

	bool Load(const wchar_t* basePath, const wchar_t* moduleSettings, ModuleLoadResult& loadResult);
	void Unload();
	
	void AddExtensionFilter(std::wstring& filterStr);
	bool DoesPathMatchFilter(const wchar_t* path) const;

	const wchar_t* Name() const { return m_sModuleName.c_str(); }
	const wchar_t* LibraryFile() const { return m_sLibraryFile.c_str(); }

private:
	std::wstring m_sModuleName;
	std::wstring m_sLibraryFile;

	HMODULE m_hModuleHandle;

	LoadSubModuleFunc m_pLoadModule;
	UnloadSubModuleFunc m_pUnloadModule;
	
	GUID m_ModuleId;
	DWORD m_nModuleVersion;

	ExtensionsFilter m_pExtensionFilter;

	ExternalModule() = delete;
	ExternalModule(const ExternalModule& other) = delete;
	ExternalModule &operator=(const ExternalModule &a) = delete;

	bool IsModuleOk(const ModuleLoadParameters &params);
};

#endif // ExternalModule_h__
