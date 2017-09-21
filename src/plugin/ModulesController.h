#ifndef _ModulesController_h__
#define _ModulesController_h__

#include "ExternalModule.h"
#include "Config.h"

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
	std::wstring ModuleName;
	std::wstring ErrorMessage;
};

class ModulesController
{
private:
	std::vector<ExternalModule*> m_vModules;
	
	ExternalModule* LoadModule(const wchar_t* basePath, const std::wstring& moduleName, const std::wstring& moduleLibrary, ConfigSection* moduleSettings, std::wstring &errorMsg);
	bool GetExceptionMessage(unsigned long exceptionCode, std::wstring &errorText);
	bool GetExceptionMessage(unsigned long exceptionCode, wchar_t* errTextBuf, size_t errTextBufSize);
	bool GetSystemErrorMessage(DWORD errorCode, std::wstring &errorText);

public:
	ModulesController(void) {};
	~ModulesController(void) { this->Cleanup(); };

	int Init(const wchar_t* basePath, Config* cfg, std::vector<FailedModuleInfo> &failed);
	void Cleanup();
	
	size_t NumModules() const { return m_vModules.size(); }
	const ExternalModule* GetModule(int index) { return m_vModules[index]; }

	int OpenStorageFile(OpenStorageFileInParams srcParams, int *moduleIndex, HANDLE *storage, StorageGeneralInfo *info);
	void CloseStorageFile(int moduleIndex, HANDLE storage);
};

#endif // _ModulesController_h__