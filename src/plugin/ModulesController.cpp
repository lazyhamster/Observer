#include "StdAfx.h"
#include "ModulesController.h"
#include "CommonFunc.h"

// Separate function is require because if __try use below, it complains about object unwinding
static void GetInvalidApiErrorMessage(std::wstring &dest, DWORD moduleVersion)
{
	dest = FormatString(L"Invalid API version (reported: %d, required %d)", moduleVersion, ACTUAL_API_VERSION);
}

int ModulesController::Init( const wchar_t* basePath, Config* cfg, std::vector<FailedModuleInfo> &failed )
{
	Cleanup();

	// Get list of modules from config file
	ConfigSection *mModulesList = cfg->GetSection(L"Modules");
	if (!mModulesList) return 0;

	ConfigSection *mFiltersList = cfg->GetSection(L"Filters");
	ConfigSection *mShortcutList = cfg->GetSection(L"Shortcuts");
	bool vUsedShortcuts[36] = {0};

	for (size_t i = 0; i < mModulesList->Count(); i++)
	{
		const ConfigItem& nextModuleInfo = mModulesList->GetItem(i);
		if (nextModuleInfo.Key.size() == 0 || nextModuleInfo.Value.size() == 0)
			continue;

		ExternalModule module(nextModuleInfo.Key, nextModuleInfo.Value);
		
		ConfigSection* moduleOpts = cfg->GetSection(module.Name());
		wstring moduleLoadError;

		if (LoadModule(basePath, module, moduleOpts, moduleLoadError))
		{
			// Read extensions filter for each module
			if (mFiltersList != NULL)
			{
				wstring extList;
				if (mFiltersList->GetValue(module.Name(), extList))
				{
					module.AddExtensionFilter(extList);
				}
			}

			// Assign shortcut
			if (mShortcutList != NULL)
			{
				wchar_t cShortcut;
				if (mShortcutList->GetValue(module.Name(), cShortcut))
				{
					cShortcut = towupper(cShortcut);
					module.ShortCut = cShortcut;
					if (iswalpha(cShortcut))
						vUsedShortcuts[cShortcut - L'A' + 10] = true;
					else
						vUsedShortcuts[cShortcut - L'0'] = true;
				}
			}
			
			modules.push_back(module);
		}
		else
		{
			FailedModuleInfo failInfo;
			failInfo.ModuleName = module.Name();
			failInfo.ErrorMessage = moduleLoadError;

			failed.push_back(failInfo);
		}
	} // for

	// Assign automatic shortcuts to modules without one
	size_t lastScIndex = 1;
	for (size_t j = 0; j < modules.size(); j++)
	{
		ExternalModule &module = modules[j];
		if (module.ShortCut) continue;

		// Find first unused shortcut
		while (lastScIndex < _countof(vUsedShortcuts) && vUsedShortcuts[lastScIndex])
			lastScIndex++;
		
		// Too many modules, not enough shortcuts
		if (lastScIndex >= _countof(vUsedShortcuts)) break;
		
		module.ShortCut = (lastScIndex < 10) ? (L'0' + lastScIndex) : (L'A' + lastScIndex - 10);
		vUsedShortcuts[lastScIndex] = true;
		lastScIndex++;
	}

	return (int) modules.size();
}

void ModulesController::Cleanup()
{
	for (size_t i = 0; i < modules.size(); i++)
	{
		ExternalModule &modulePtr = modules[i];
		modulePtr.Unload();
	}
	modules.clear();
}

bool ModulesController::LoadModule(const wchar_t* basePath, ExternalModule &module, ConfigSection* moduleSettings, std::wstring &errorMsg)
{
	ModuleLoadResult loadResult;
	wstring moduleSettingsStr = moduleSettings ? moduleSettings->GetAll() : L"";

	if (!module.Load(basePath, moduleSettingsStr.c_str(), loadResult))
	{
		switch (loadResult.Status)
		{
		case ModuleLoadStatus::LoadLibraryFailed:
			GetSystemErrorMessage(loadResult.ErrorCode, errorMsg);
			break;
		case ModuleLoadStatus::ExceptionCaught:
			GetExceptionMessage(loadResult.ErrorCode, errorMsg);
			break;
		case ModuleLoadStatus::InvalidAPIVersion:
			errorMsg = FormatString(L"Invalid API version (reported: %d, required %d)", loadResult.ErrorCode, ACTUAL_API_VERSION);
			break;
		case ModuleLoadStatus::LoadModuleFailed:
			errorMsg = L"Module was loaded successfully but LoadSubModule returned an error";
			break;
		case ModuleLoadStatus::NotModule:
			errorMsg = FormatString(L"Library %s is not a valid Observer module", module.LibraryFile());
			break;
		default:
			errorMsg = L"[ATTENTION] Unrecognized error on module load attempt";
			break;
		}

		return false;
	}

	return true;
}

int ModulesController::OpenStorageFile(OpenStorageFileInParams srcParams, int *moduleIndex, HANDLE *storage, StorageGeneralInfo *sinfo)
{
	// Check input params
	if (!moduleIndex || !sinfo || !storage)
		return SOR_INVALID_FILE;

	StorageOpenParams openParams = {0};
	openParams.StructSize = sizeof(StorageOpenParams);
	openParams.FilePath = srcParams.path;
	openParams.Password = srcParams.password;
	openParams.Data = srcParams.dataBuffer;
	openParams.DataSize = srcParams.dataSize;
	
	*moduleIndex = -1;
	for (size_t i = 0; i < modules.size(); i++)
	{
		if (srcParams.openWithModule == -1 || srcParams.openWithModule == i)
		{
			const ExternalModule &modulePtr = modules[i];
			if (!srcParams.applyExtFilters || modulePtr.DoesPathMatchFilter(srcParams.path))
			{
				int openRes;
				__try
				{
					openRes = modulePtr.ModuleFunctions.OpenStorage(openParams, storage, sinfo);
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					//NOTE: Maybe module that produced exception should be unloaded
#ifdef _DEBUG
					wchar_t buf[1024] = {0};
					GetExceptionMessage(_exception_code(), buf, ARRAY_SIZE(buf));
					MessageBox(0, buf, L"Open file error", MB_OK);
#endif
					continue;
				}

				if (openRes != SOR_INVALID_FILE)
				{
					*moduleIndex = (int) i;
					return openRes;
				}
			}
		}
	} // for i

	return SOR_INVALID_FILE;
}

void ModulesController::CloseStorageFile(int moduleIndex, HANDLE storage)
{
	if ((moduleIndex >= 0) && (moduleIndex < (int) modules.size()))
	{
		const ExternalModule &modulePtr = modules[moduleIndex];
		modulePtr.ModuleFunctions.CloseStorage(storage);
	}
}

bool ModulesController::GetExceptionMessage(unsigned long exceptionCode, std::wstring &errorText)
{
	//TODO: this code is not working properly, should re-implement
	
	bool rval = true;
	
	wchar_t* msgBuffer = NULL;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandle(L"NTDLL.dll"), exceptionCode, 0, msgBuffer, 0, NULL) == 0)
	{
		const size_t kMsgBufferSize = 256;
		msgBuffer = (wchar_t*) LocalAlloc(LPTR, kMsgBufferSize * sizeof(wchar_t));
		if (exceptionCode == 0xE0434352)
			swprintf_s(msgBuffer, kMsgBufferSize, L".NET code has thrown an exception (%u)", exceptionCode);
		else
			swprintf_s(msgBuffer, kMsgBufferSize, L"Unrecognized exception: %u", exceptionCode);
		rval = false;
	}
	errorText = msgBuffer;
	LocalFree(msgBuffer);

	return rval;
}

bool ModulesController::GetExceptionMessage(unsigned long exceptionCode, wchar_t* errTextBuf, size_t errTextBufSize)
{
	std::wstring str;
	bool rval = GetExceptionMessage(exceptionCode, str);
	wcscpy_s(errTextBuf, errTextBufSize, str.c_str());
	return rval;
}

bool ModulesController::GetSystemErrorMessage(DWORD errorCode, std::wstring &errorText)
{
	bool rval = true;
	
	wchar_t* msgBuffer = NULL;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, 0, (LPWSTR)&msgBuffer, 0, NULL) == 0)
	{
		msgBuffer = (wchar_t*)LocalAlloc(LPTR, 256 * sizeof(wchar_t));
		swprintf_s(msgBuffer, LocalSize(msgBuffer) / sizeof(*msgBuffer), L"Unrecognized error code: %u", errorCode);
		rval = false;
	}
	errorText = msgBuffer;
	LocalFree(msgBuffer);

	return rval;
}
