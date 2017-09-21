#include "stdafx.h"
#include "ExternalModule.h"

ExternalModule::ExternalModule(const std::wstring& Name, const std::wstring& Library)
{
	m_sModuleName = Name;
	m_sLibraryFile = Library;
	
	m_pLoadModule = nullptr;
	m_pUnloadModule = nullptr;
	m_hModuleHandle = NULL;
	
	m_nModuleVersion = 0;
	m_ModuleId = GUID_NULL;
	memset(&ModuleFunctions, 0, sizeof(ModuleFunctions));
	ShortCut = '\0';
}

ExternalModule::~ExternalModule()
{
	//Unload();
}

bool ExternalModule::Load(const wchar_t* basePath, const wchar_t* moduleSettings, ModuleLoadResult& loadResult)
{
	loadResult.Status = ModuleLoadStatus::Success;
	loadResult.ErrorCode = 0;
	
	wchar_t wszFullModulePath[MAX_PATH] = { 0 };
	swprintf_s(wszFullModulePath, MAX_PATH, L"%s%s", basePath, m_sLibraryFile.c_str());

	HMODULE hMod = LoadLibraryEx(wszFullModulePath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (hMod != NULL)
	{
		// Search for exported functions
		m_pLoadModule = (LoadSubModuleFunc)GetProcAddress(hMod, "LoadSubModule");
		m_pUnloadModule = (UnloadSubModuleFunc)GetProcAddress(hMod, "UnloadSubModule");

		// Try to load modules
		if ((m_pLoadModule != nullptr) && (m_pUnloadModule != nullptr))
		{
			m_hModuleHandle = hMod;

			ModuleLoadParameters loadParams = { 0 };
			loadParams.StructSize = sizeof(ModuleLoadParameters);
			loadParams.Settings = moduleSettings;

			__try
			{
				if (m_pLoadModule(&loadParams) && IsModuleOk(loadParams))
				{
					if (loadParams.ApiVersion != ACTUAL_API_VERSION)
					{
						loadResult.Status = ModuleLoadStatus::InvalidAPIVersion;
						loadResult.ErrorCode = loadParams.ApiVersion;
						m_pUnloadModule();
					}
					else
					{
						ModuleFunctions = loadParams.ApiFuncs;
						m_ModuleId = loadParams.ModuleId;
						m_nModuleVersion = loadParams.ModuleVersion;
					}
				}
				else
				{
					loadResult.Status = ModuleLoadStatus::LoadModuleFailed;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				loadResult.Status = ModuleLoadStatus::ExceptionCaught;
				loadResult.ErrorCode = GetExceptionCode();
			}
		}
		else
		{
			loadResult.Status = ModuleLoadStatus::NotModule;
		}
		
		if (loadResult.Status != ModuleLoadStatus::Success)
		{
			m_pLoadModule = nullptr;
			m_pUnloadModule = nullptr;
			FreeLibrary(hMod);
		}
	}
	else
	{
		loadResult.Status = ModuleLoadStatus::LoadLibraryFailed;
		loadResult.ErrorCode = GetLastError();
	}
	
	return (loadResult.Status == ModuleLoadStatus::Success);
}

void ExternalModule::Unload()
{
	if (m_pUnloadModule)
		m_pUnloadModule();

	if (m_hModuleHandle)
		FreeLibrary(m_hModuleHandle);
	
	m_pLoadModule = nullptr;
	m_pUnloadModule = nullptr;
	m_hModuleHandle = NULL;
	m_nModuleVersion = 0;
	
	m_ModuleId = GUID_NULL;
	memset(&ModuleFunctions, 0, sizeof(ModuleFunctions));
}

bool ExternalModule::IsModuleOk(const ModuleLoadParameters &params)
{
	return (params.ApiFuncs.OpenStorage != nullptr) && (params.ApiFuncs.CloseStorage != nullptr)
		&& (params.ApiFuncs.PrepareFiles != nullptr) && (params.ApiFuncs.GetItem != nullptr) && (params.ApiFuncs.ExtractItem != nullptr);
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
