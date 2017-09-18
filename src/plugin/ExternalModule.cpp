#include "stdafx.h"
#include "ExternalModule.h"

ExternalModule::ExternalModule(const std::wstring& Name, const std::wstring& Library)
{
	m_sModuleName = Name;
	m_sLibraryFile = Library;
	
	LoadModule = nullptr;
	UnloadModule = nullptr;
	ModuleHandle = 0;
	ModuleVersion = 0;
	ModuleId = GUID_NULL;
	memset(&ModuleFunctions, 0, sizeof(ModuleFunctions));
	ShortCut = '\0';
}

int ExternalModule::AddExtensionFilter(std::wstring& filterStr)
{
	if (filterStr.size() == 0) return 0;

	wchar_t* context = NULL;
	wchar_t* strList = _wcsdup(filterStr.c_str());
	int numAdded = 0;

	wchar_t* token = wcstok_s(strList, L";", &context);
	while (token)
	{
		if (wcslen(token) > 0)
		{
			while (!token[0]) token++;
			m_vExtensionFilter.push_back(token);
			numAdded++;
		}

		token = wcstok_s(NULL, L";", &context);
	}
	free(strList);

	return numAdded;
}

bool ExternalModule::DoesPathMatchFilter(const wchar_t* path) const
{
	// Empty filter means match all
	if (m_vExtensionFilter.size() == 0)
		return true;

	for (size_t i = 0; i < m_vExtensionFilter.size(); ++i)
	{
		if (PathMatchSpec(path, m_vExtensionFilter[i].c_str()))
			return true;
	}

	return false;
}
