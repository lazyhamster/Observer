#include "stdafx.h"
#include "WcxModule.h"

bool IWcxModule::Load( HMODULE module, const wchar_t* moduleName, int codePage )
{
	modCloseArchive = (CloseArchiveFunc) GetProcAddress(module, "CloseArchive");
	if (modCloseArchive == NULL) return false;
	
	if (InternalInit(m_Module))
	{
		m_Module = module;
		m_ModuleName = moduleName;
		m_DefaultCodePage = codePage;

		return true;
	}

	return false;
}

void IWcxModule::Unload()
{
	FreeModule(m_Module);
}

//////////////////////////////////////////////////////////////////////////

bool WcxUnicodeModule::InternalInit(HMODULE module)
{
	modCanYouHandleThisFile = (CanYouHandleThisFileWFunc) GetProcAddress(module, "CanYouHandleThisFileW");
	modOpenArchive = (OpenArchiveWFunc) GetProcAddress(module, "OpenArchiveW");
	modProcessFile = (ProcessFileWFunc) GetProcAddress(module, "ProcessFileW");
	modReadHeader = (ReadHeaderExWFunc) GetProcAddress(module, "ReadHeaderExW");
	modSetProcessDataProc = (SetProcessDataProcWFunc) GetProcAddress(module, "SetProcessDataProcW");

	return (modOpenArchive != NULL) && (modProcessFile != NULL) && (modReadHeader != NULL) && (modSetProcessDataProc != NULL);
}

bool WcxUnicodeModule::IsArchive(const wchar_t* wszFilePath)
{
	return (modCanYouHandleThisFile == NULL) || (modCanYouHandleThisFile(wszFilePath) != FALSE);
}

HANDLE WcxUnicodeModule::OpenArchive(const wchar_t* wszFilePath, int nOpMode)
{
	//TODO: implement
	return 0;
}

int WcxUnicodeModule::ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData)
{
	return modReadHeader(hArchive, HeaderData);
}

int WcxUnicodeModule::ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName)
{
	return modProcessFile(hArchive, Operation, DestPath, DestName);
}

//////////////////////////////////////////////////////////////////////////

bool WcxAnsiExtModule::InternalInit(HMODULE module)
{
	modCanYouHandleThisFile = (CanYouHandleThisFileFunc) GetProcAddress(module, "CanYouHandleThisFile");
	modOpenArchive = (OpenArchiveFunc) GetProcAddress(module, "OpenArchive");
	modProcessFile = (ProcessFileFunc) GetProcAddress(module, "ProcessFile");
	modReadHeader = (ReadHeaderExFunc) GetProcAddress(module, "ReadHeaderEx");
	modSetProcessDataProc = (SetProcessDataProcFunc) GetProcAddress(module, "SetProcessDataProc");
	
	return (modOpenArchive != NULL) && (modProcessFile != NULL) && (modReadHeader != NULL) && (modSetProcessDataProc != NULL);
}

bool WcxAnsiExtModule::IsArchive(const wchar_t* wszFilePath)
{
	char szAnsiFilePath[MAX_PATH] = {0};
	int ret = WideCharToMultiByte(m_DefaultCodePage, 0, wszFilePath, -1, szAnsiFilePath, MAX_PATH, NULL, NULL);
	
	return (ret > 0) && (modCanYouHandleThisFile == NULL || modCanYouHandleThisFile(szAnsiFilePath) != FALSE);
}

HANDLE WcxAnsiExtModule::OpenArchive(const wchar_t* wszFilePath, int nOpMode)
{
	//TODO: implement
	return 0;
}

int WcxAnsiExtModule::ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData)
{
	//TODO: implement
	return 0;
}

int WcxAnsiExtModule::ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName)
{
	//TODO: implement
	return 0;
}

//////////////////////////////////////////////////////////////////////////

bool WcxAnsiModule::InternalInit(HMODULE module)
{
	modCanYouHandleThisFile = (CanYouHandleThisFileFunc) GetProcAddress(module, "CanYouHandleThisFile");
	modOpenArchive = (OpenArchiveFunc) GetProcAddress(module, "OpenArchive");
	modProcessFile = (ProcessFileFunc) GetProcAddress(module, "ProcessFile");
	modReadHeader = (ReadHeaderFunc) GetProcAddress(module, "ReadHeader");
	modSetProcessDataProc = (SetProcessDataProcFunc) GetProcAddress(module, "SetProcessDataProc");
	
	return (modOpenArchive != NULL)	&& (modProcessFile != NULL) && (modReadHeader != NULL) && (modSetProcessDataProc != NULL);
}

bool WcxAnsiModule::IsArchive(const wchar_t* wszFilePath)
{
	char szAnsiFilePath[MAX_PATH] = {0};
	int ret = WideCharToMultiByte(m_DefaultCodePage, 0, wszFilePath, -1, szAnsiFilePath, MAX_PATH, NULL, NULL);

	return (ret > 0) && (modCanYouHandleThisFile == NULL || modCanYouHandleThisFile(szAnsiFilePath) != FALSE);
}

HANDLE WcxAnsiModule::OpenArchive(const wchar_t* wszFilePath, int nOpMode)
{
	//TODO: implement
	return 0;
}

int WcxAnsiModule::ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData)
{
	//TODO: implement
	return 0;
}

int WcxAnsiModule::ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName)
{
	//TODO: implement
	return 0;
}
