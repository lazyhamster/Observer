#include "StdAfx.h"
#include "ModulesController.h"
#include "CommonFunc.h"

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
		if (nextModuleInfo.Key.empty() || nextModuleInfo.Value.empty())
			continue;

		const std::wstring& moduleName = nextModuleInfo.Key;
		const std::wstring& moduleLibrary = nextModuleInfo.Value;

		ConfigSection* moduleOpts = cfg->GetSection(moduleName);
		std::wstring moduleLoadError;

		ExternalModule* module = LoadModule(basePath, moduleName, moduleLibrary, moduleOpts, moduleLoadError);
		if (module)
		{
			// Read extensions filter for each module
			if (mFiltersList != NULL)
			{
				std::wstring extList;
				if (mFiltersList->GetValue(module->Name(), extList))
				{
					module->AddExtensionFilter(extList);
				}
			}

			// Assign shortcut
			if (mShortcutList != NULL)
			{
				wchar_t cShortcut;
				if (mShortcutList->GetValue(module->Name(), cShortcut))
				{
					cShortcut = towupper(cShortcut);
					module->ShortCut = cShortcut;
					if (iswalpha(cShortcut))
						vUsedShortcuts[cShortcut - L'A' + 10] = true;
					else
						vUsedShortcuts[cShortcut - L'0'] = true;
				}
			}
			
			m_vModules.push_back(module);
		}
		else
		{
			FailedModuleInfo failInfo;
			failInfo.ModuleName = moduleName;
			failInfo.ErrorMessage = moduleLoadError;

			failed.push_back(failInfo);
		}
	} // for

	// Assign automatic shortcuts to modules without one
	unsigned short lastScIndex = 1;
	for (size_t j = 0; j < m_vModules.size(); j++)
	{
		ExternalModule* module = m_vModules[j];
		if (module->ShortCut) continue;

		// Find first unused shortcut
		while (lastScIndex < _countof(vUsedShortcuts) && vUsedShortcuts[lastScIndex])
			lastScIndex++;
		
		// Too many modules, not enough shortcuts
		if (lastScIndex >= _countof(vUsedShortcuts)) break;
		
		module->ShortCut = (lastScIndex < 10) ? (L'0' + lastScIndex) : (L'A' + lastScIndex - 10);
		vUsedShortcuts[lastScIndex] = true;
		lastScIndex++;
	}

	return (int)m_vModules.size();
}

void ModulesController::Cleanup()
{
	for (size_t i = 0; i < m_vModules.size(); i++)
	{
		ExternalModule *modulePtr = m_vModules[i];
		
		modulePtr->Unload();
		delete modulePtr;
	}
	m_vModules.clear();
}

ExternalModule* ModulesController::LoadModule(const wchar_t* basePath, const std::wstring& moduleName, const std::wstring& moduleLibrary, ConfigSection* moduleSettings, std::wstring &errorMsg)
{
	ModuleLoadResult loadResult;
	std::wstring moduleSettingsStr = moduleSettings ? moduleSettings->GetAll() : L"";

	ExternalModule* module = new ExternalModule(moduleName, moduleLibrary);

	if (!module->Load(basePath, moduleSettingsStr.c_str(), loadResult))
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
			errorMsg = FormatString(L"Library %s is not a valid Observer module", moduleLibrary.c_str());
			break;
		default:
			errorMsg = L"[ATTENTION] Unrecognized error on module load attempt";
			break;
		}

		delete module;
		return nullptr;
	}

	return module;
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
	for (size_t i = 0; i < m_vModules.size(); i++)
	{
		if (srcParams.openWithModule == -1 || srcParams.openWithModule == i)
		{
			const ExternalModule* modulePtr = m_vModules[i];
			if (!srcParams.applyExtFilters || modulePtr->DoesPathMatchFilter(srcParams.path))
			{
				int openRes;
				__try
				{
					openRes = modulePtr->ModuleFunctions.OpenStorage(openParams, storage, sinfo);
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
	if ((moduleIndex >= 0) && (moduleIndex < (int)m_vModules.size()))
	{
		const ExternalModule* modulePtr = m_vModules[moduleIndex];
		modulePtr->ModuleFunctions.CloseStorage(storage);
	}
}

bool ModulesController::GetExceptionMessage(unsigned long exceptionCode, std::wstring &errorText)
{
	bool rval = true;

	switch (exceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		errorText = L"The thread attempts to read from or write to a virtual address for which it does not have access.";
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		errorText = L"The thread attempts to access an array element that is out of bounds, and the underlying hardware supports bounds checking.";
		break;
	case EXCEPTION_BREAKPOINT:
		errorText = L"A breakpoint is encountered.";
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		errorText = L"The thread attempts to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries, 32-bit values on 4-byte boundaries, and so on.";
		break;
	case EXCEPTION_GUARD_PAGE:
		errorText = L"The thread accessed memory allocated with the PAGE_GUARD modifier.";
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		errorText = L"The thread tries to execute an invalid instruction.";
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		errorText = L"The thread tries to access a page that is not present, and the system is unable to load the page. For example, this exception might occur if a network connection is lost while running a program over a network.";
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		errorText = L"The thread attempts to divide an integer value by an integer divisor of 0 (zero).";
		break;
	case EXCEPTION_INT_OVERFLOW:
		errorText = L"The result of an integer operation creates a value that is too large to be held by the destination register. In some cases, this will result in a carry out of the most significant bit of the result. Some operations do not set the carry flag.";
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		errorText = L"An exception handler returns an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.";
		break;
	case EXCEPTION_INVALID_HANDLE:
		errorText = L"The thread used a handle to a kernel object that was invalid (probably because it had been closed.)";
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		errorText = L"The thread attempts to continue execution after a non-continuable exception occurs.";
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		errorText = L"The thread attempts to execute an instruction with an operation that is not allowed in the current computer mode.";
		break;
	case EXCEPTION_SINGLE_STEP:
		errorText = L"A trace trap or other single instruction mechanism signals that one instruction is executed.";
		break;
	case EXCEPTION_STACK_OVERFLOW:
		errorText = L"The thread uses up its stack.";
		break;
	case 0xE0434352:
		errorText = L".NET code has thrown an exception";
		break;
	default:
		errorText = FormatString(L"Unrecognized exception: %u", exceptionCode);
		rval = false;
		break;
	}
	
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
