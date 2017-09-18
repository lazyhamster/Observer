#ifndef ExternalModule_h__
#define ExternalModule_h__

#include "ModuleDef.h"

struct ExternalModule
{
	HMODULE ModuleHandle;
	
	LoadSubModuleFunc LoadModule;
	UnloadSubModuleFunc UnloadModule;

	GUID ModuleId;
	DWORD ModuleVersion;
	module_cbs ModuleFunctions;

	wchar_t ShortCut;

	ExternalModule(const std::wstring& Name, const std::wstring& Library);
	
	int AddExtensionFilter(std::wstring& filterStr);
	bool DoesPathMatchFilter(const wchar_t* path) const;

	const wchar_t* Name() const { return m_sModuleName.c_str(); }
	const wchar_t* LibraryFile() const { return m_sLibraryFile.c_str(); }

private:
	std::wstring m_sModuleName;
	std::wstring m_sLibraryFile;

	std::vector<std::wstring> m_vExtensionFilter;

	ExternalModule() = delete;
};

#endif // ExternalModule_h__
