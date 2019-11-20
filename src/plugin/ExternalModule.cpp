#include "stdafx.h"
#include "ExternalModule.h"

ExternalModule::ExternalModule(const std::wstring& Name, const std::wstring& Library)
	: m_pExtensionFilter(true), m_sModuleName(Name), m_sLibraryFile(Library)
{
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
	Unload();
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

void ExternalModule::AddExtensionFilter(const std::wstring& filterStr)
{
	m_pExtensionFilter.SetFilter(filterStr);
}

bool ExternalModule::DoesPathMatchFilter(const wchar_t* path) const
{
	return m_pExtensionFilter.DoesPathMatch(path);
}
