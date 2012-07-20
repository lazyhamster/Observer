#include "stdafx.h"
#include "WcxModule.h"

bool IWcxModule::Load( HMODULE module, const wchar_t* moduleName, int codePage )
{
	modCloseArchive = (CloseArchiveFunc) GetProcAddress(module, "CloseArchive");
	if (modCloseArchive == NULL) return false;
	
	if (InternalInit(module))
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
	modSetChangeVolProc = (SetChangeVolProcWFunc) GetProcAddress(module, "SetChangeVolProcW");

	return (modOpenArchive != NULL) && (modProcessFile != NULL) && (modReadHeader != NULL) && (modSetProcessDataProc != NULL) && (modSetChangeVolProc != NULL);
}

bool WcxUnicodeModule::IsArchive(const wchar_t* wszFilePath)
{
	return (modCanYouHandleThisFile == NULL) || (modCanYouHandleThisFile(wszFilePath) != FALSE);
}

HANDLE WcxUnicodeModule::OpenArchive(const wchar_t* wszFilePath, int nOpMode)
{
	tOpenArchiveDataW arcData = {0};
	arcData.ArcName = const_cast<wchar_t*>(wszFilePath);
	arcData.OpenMode = nOpMode;
	
	HANDLE hArcData = modOpenArchive(&arcData);

	if (hArcData != NULL)
	{
		modSetProcessDataProc(hArcData, WcxUnicodeModule::ProcessDataProcW);
		modSetChangeVolProc(hArcData, WcxUnicodeModule::ChangeVolProcW);
	}

	return hArcData;
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

bool WcxAnsiModule::InternalInit(HMODULE module)
{
	modCanYouHandleThisFile = (CanYouHandleThisFileFunc) GetProcAddress(module, "CanYouHandleThisFile");
	modOpenArchive = (OpenArchiveFunc) GetProcAddress(module, "OpenArchive");
	modProcessFile = (ProcessFileFunc) GetProcAddress(module, "ProcessFile");
	modReadHeader = (ReadHeaderFunc) GetProcAddress(module, "ReadHeader");
	modSetProcessDataProc = (SetProcessDataProcFunc) GetProcAddress(module, "SetProcessDataProc");
	modSetChangeVolProc = (SetChangeVolProcFunc) GetProcAddress(module, "SetChangeVolProc");
	
	return (modOpenArchive != NULL)	&& (modProcessFile != NULL) && (modReadHeader != NULL) && (modSetProcessDataProc != NULL) && (modSetChangeVolProc != NULL);
}

bool WcxAnsiModule::IsArchive(const wchar_t* wszFilePath)
{
	char szAnsiFilePath[MAX_PATH] = {0};
	int ret = WideCharToMultiByte(m_DefaultCodePage, 0, wszFilePath, -1, szAnsiFilePath, MAX_PATH, NULL, NULL);

	return (ret > 0) && (modCanYouHandleThisFile == NULL || modCanYouHandleThisFile(szAnsiFilePath) != FALSE);
}

HANDLE WcxAnsiModule::OpenArchive(const wchar_t* wszFilePath, int nOpMode)
{
	char szAnsiPath[MAX_PATH] = {0};
	WideCharToMultiByte(m_DefaultCodePage, 0, wszFilePath, -1, szAnsiPath, MAX_PATH, NULL, NULL);

	tOpenArchiveData arcData = {0};
	arcData.ArcName = szAnsiPath;
	arcData.OpenMode = nOpMode;
	
	HANDLE hArcData = modOpenArchive(&arcData);

	if (hArcData != NULL)
	{
		modSetProcessDataProc(hArcData, WcxAnsiModule::ProcessDataProc);
		modSetChangeVolProc(hArcData, WcxAnsiModule::ChangeVolProc);
	}

	return hArcData;
}

int WcxAnsiModule::ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData)
{
	tHeaderData ansiHeaderData = {0};
	int retVal = modReadHeader(hArchive, &ansiHeaderData);

	if (retVal == 0)
	{
		memset(HeaderData, 0, sizeof(*HeaderData));

		MultiByteToWideChar(m_DefaultCodePage, 0, ansiHeaderData.ArcName, -1, HeaderData->ArcName, sizeof(HeaderData->ArcName) / sizeof(HeaderData->ArcName[0]));
		MultiByteToWideChar(m_DefaultCodePage, 0, ansiHeaderData.FileName, -1, HeaderData->FileName, sizeof(HeaderData->FileName) / sizeof(HeaderData->FileName[0]));
		HeaderData->FileAttr = ansiHeaderData.FileAttr;
		HeaderData->FileCRC = ansiHeaderData.FileCRC;
		HeaderData->FileTime = ansiHeaderData.FileTime;
		HeaderData->Flags = ansiHeaderData.Flags;
		HeaderData->HostOS = ansiHeaderData.HostOS;
		HeaderData->Method = ansiHeaderData.Method;
		HeaderData->PackSize = ansiHeaderData.PackSize;
		HeaderData->PackSizeHigh = 0;
		HeaderData->UnpSize = ansiHeaderData.UnpSize;
		HeaderData->UnpSizeHigh = 0;
		HeaderData->UnpVer = ansiHeaderData.UnpVer;
	}

	return retVal;
}

int WcxAnsiModule::ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName)
{
	char ansiDestPath[MAX_PATH] = {0};
	char ansiDestName[MAX_PATH] = {0};

	if (DestPath != NULL)
		WideCharToMultiByte(m_DefaultCodePage, 0, DestPath, -1, ansiDestPath, sizeof(ansiDestPath), NULL, NULL);
	if (DestName != NULL)
		WideCharToMultiByte(m_DefaultCodePage, 0, DestName, -1, ansiDestName, sizeof(ansiDestName), NULL, NULL);

	return modProcessFile(hArchive, Operation, DestPath ? ansiDestPath : NULL, DestName ? ansiDestName : NULL);
}

//////////////////////////////////////////////////////////////////////////

bool WcxAnsiExtModule::InternalInit(HMODULE module)
{
	modCanYouHandleThisFile = (CanYouHandleThisFileFunc) GetProcAddress(module, "CanYouHandleThisFile");
	modOpenArchive = (OpenArchiveFunc) GetProcAddress(module, "OpenArchive");
	modProcessFile = (ProcessFileFunc) GetProcAddress(module, "ProcessFile");
	modReadHeaderEx = (ReadHeaderExFunc) GetProcAddress(module, "ReadHeaderEx");
	modSetProcessDataProc = (SetProcessDataProcFunc) GetProcAddress(module, "SetProcessDataProc");

	return (modOpenArchive != NULL) && (modProcessFile != NULL) && (modReadHeaderEx != NULL) && (modSetProcessDataProc != NULL);
}

int WcxAnsiExtModule::ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData)
{
	tHeaderDataEx ansiHeaderData = {0};
	int retVal = modReadHeaderEx(hArchive, &ansiHeaderData);

	if (retVal == 0)
	{
		memset(HeaderData, 0, sizeof(*HeaderData));

		MultiByteToWideChar(m_DefaultCodePage, 0, ansiHeaderData.ArcName, -1, HeaderData->ArcName, sizeof(HeaderData->ArcName) / sizeof(HeaderData->ArcName[0]));
		MultiByteToWideChar(m_DefaultCodePage, 0, ansiHeaderData.FileName, -1, HeaderData->FileName, sizeof(HeaderData->FileName) / sizeof(HeaderData->FileName[0]));
		HeaderData->FileAttr = ansiHeaderData.FileAttr;
		HeaderData->FileCRC = ansiHeaderData.FileCRC;
		HeaderData->FileTime = ansiHeaderData.FileTime;
		HeaderData->Flags = ansiHeaderData.Flags;
		HeaderData->HostOS = ansiHeaderData.HostOS;
		HeaderData->Method = ansiHeaderData.Method;
		HeaderData->PackSize = ansiHeaderData.PackSize;
		HeaderData->PackSizeHigh = ansiHeaderData.PackSizeHigh;
		HeaderData->UnpSize = ansiHeaderData.UnpSize;
		HeaderData->UnpSizeHigh = ansiHeaderData.UnpSizeHigh;
		HeaderData->UnpVer = ansiHeaderData.UnpVer;
	}

	return retVal;
}
