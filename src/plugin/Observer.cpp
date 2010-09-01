// Observer.cpp : Defines the exported functions for the DLL application.
// This module contains functions for ANSI version of FAR (1.75+)

#include "StdAfx.h"
#include <far/plugin.hpp>

#include "CommonFunc.h"
#include "ModulesController.h"
#include "PlugLang.h"
#include "FarStorage.h"

#include "OptionsParser.h"
#include "RegistrySettings.h"

extern HMODULE g_hDllHandle;
static PluginStartupInfo FarSInfo;
static FarStandardFunctions FSF;

static wchar_t wszPluginLocation[MAX_PATH];
static ModulesController g_pController;

#define CP_FAR_INTERNAL CP_OEMCP

// Settings
#define MAX_PREFIX_SIZE 32
static int optEnabled = TRUE;
static int optUsePrefix = TRUE;
static char optPrefix[MAX_PREFIX_SIZE] = "observe";

static char optPanelHeaderPrefix[MAX_PREFIX_SIZE] = "";

//-----------------------------------  Local functions ----------------------------------------

struct InitDialogItem
{
	int Type;
	int X1;
	int Y1;
	int X2;
	int Y2;
	int Focus;
	int Selected;
	unsigned int Flags;
	int DefaultButton;
	char *Data;
};


static void InitDialogItems(const struct InitDialogItem *Init, struct FarDialogItem *Item,	int ItemsNumber)
{
	int I;
	const struct InitDialogItem *PInit=Init;
	struct FarDialogItem *PItem=Item;
	for (I=0; I < ItemsNumber; I++,PItem++,PInit++)
	{
		PItem->Type=PInit->Type;
		PItem->X1=PInit->X1;
		PItem->Y1=PInit->Y1;
		PItem->X2=PInit->X2;
		PItem->Y2=PInit->Y2;
		PItem->Focus=PInit->Focus;
		PItem->Selected=PInit->Selected;
		PItem->Flags=PInit->Flags;
		PItem->DefaultButton=PInit->DefaultButton;
		strcpy_s(PItem->Data, sizeof(PItem->Data) / sizeof(PItem->Data[0]), PInit->Data);
	}
}

static const char* GetLocMsg(int MsgID)
{
	return FarSInfo.GetMsg(FarSInfo.ModuleNumber, MsgID);
}

static void LoadSettings()
{
	// Load static settings from .ini file.
	OptionsFile opFile(wszPluginLocation);
	OptionsList *opList = opFile.GetSection(L"General");
	if (opList != NULL)
	{
		opList->GetValue(L"PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
		delete opList;
	}

	// Load dynamic settings from registry (they will overwrite static ones)
	RegistrySettings regOpts(FarSInfo.RootKey);
	if (regOpts.Open())
	{
		regOpts.GetValue(L"Enabled", optEnabled);
		regOpts.GetValue(L"UsePrefix", optUsePrefix);
		regOpts.GetValue("Prefix", optPrefix, MAX_PREFIX_SIZE);

		regOpts.GetValue("PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
	}
}

static void SaveSettings()
{
	RegistrySettings regOpts(FarSInfo.RootKey);
	if (regOpts.Open(true))
	{
		regOpts.SetValue(L"Enabled", optEnabled);
		regOpts.SetValue(L"UsePrefix", optUsePrefix);
		regOpts.SetValue("Prefix", optPrefix);
	}
}

static void InsertCommas(char *Dest)
{
  int I;
  for (I=strlen(Dest)-4;I>=0;I-=3)
    if (Dest[I])
    {
      memmove(Dest+I+2,Dest+I+1,strlen(Dest+I));
      Dest[I+1]=',';
    }
}

static void DisplayMessage(bool isError, bool isInteractive, int headerMsgID, int textMsgID, const char* errorItem)
{
	static const char* MsgLines[3];
	MsgLines[0] = GetLocMsg(headerMsgID);
	MsgLines[1] = GetLocMsg(textMsgID);
	MsgLines[2] = errorItem;

	int linesNum = (errorItem) ? 3 : 2;
	int flags = 0;
	if (isError) flags |= FMSG_WARNING;
	if (isInteractive) flags |= FMSG_MB_OK;
	
	FarSInfo.Message(FarSInfo.ModuleNumber, flags, NULL, MsgLines, linesNum, 0);
}

//-----------------------------------  Content functions ----------------------------------------

static HANDLE OpenStorage(const char* Name)
{
	wchar_t wszWideName[MAX_PATH] = {0};
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, Name, strlen(Name), wszWideName, MAX_PATH);

	StorageObject *storage = new StorageObject(&g_pController);
	if (!storage->Open(wszWideName))
	{
		delete storage;
		return INVALID_HANDLE_VALUE;
	}

	HANDLE hScreen = FarSInfo.SaveScreen(0, 0, -1, -1);
	DisplayMessage(false, false, MSG_PLUGIN_NAME, MSG_OPEN_LIST, NULL);

	HANDLE hResult = INVALID_HANDLE_VALUE;
	bool listAborted;
	if (storage->ReadFileList(listAborted))
	{
		hResult = (HANDLE) storage;
	}
	else
	{
		if (!listAborted)
			DisplayMessage(true, true, MSG_OPEN_CONTENT_ERROR, MSG_OPEN_INVALID_ITEM, NULL);
		delete storage;
	}

	FarSInfo.RestoreScreen(hScreen);

	return hResult;
}

static void CloseStorage(HANDLE hStorage)
{
	StorageObject *sobj = (StorageObject*) hStorage;
	sobj->Close();
	delete sobj;
}

//-----------------------------------  Callback functions ----------------------------------------

static int CALLBACK ExtractProgress(HANDLE context)
{
	// Check for ESC pressed
	if (CheckEsc())
		return FALSE;

	ProgressContext* pc = (ProgressContext*) context;
	int nTotalProgress = (pc->nTotalSize > 0) ? (int) ((pc->nProcessedBytes * 100) / pc->nTotalSize) : 0;

	static char szFileProgressLine[100] = {0};
	sprintf_s(szFileProgressLine, 100, "File: %d/%d. Progress: %2d%% / %2d%%", pc->nCurrentFileNumber, pc->nTotalFiles, pc->nCurrentFileProgress, nTotalProgress);

	static const char* InfoLines[4];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_EXTRACTING);
	InfoLines[2] = szFileProgressLine;
	InfoLines[3] = pc->szFilePath;

	FarSInfo.Message(FarSInfo.ModuleNumber, 0, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 0);

	return TRUE;
}

static int CALLBACK ExtractStart(const ContentTreeNode* item, ProgressContext* context, HANDLE &screen)
{
	screen = FarSInfo.SaveScreen(0, 0, -1, -1);

	context->nCurrentFileNumber++;
	context->nCurrentFileProgress = 0;

	wchar_t wszSubPath[PATH_BUFFER_SIZE] = {0};
	item->GetPath(wszSubPath, PATH_BUFFER_SIZE);
		
	// Save file name for dialogs
	int nPathLen = wcslen(wszSubPath);
	wchar_t* wszSubPathPtr = wszSubPath;
	if (nPathLen > MAX_PATH) wszSubPathPtr += (nPathLen % MAX_PATH);

	memset(context->szFilePath, 0, MAX_PATH);
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, wszSubPathPtr, wcslen(wszSubPathPtr), context->szFilePath, MAX_PATH, NULL, NULL);
	
	// Shrink file path to fit on console
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hStdOut, &si))
		FSF.TruncPathStr(context->szFilePath, si.dwSize.X - 16);

	return ExtractProgress((HANDLE)context);
}

static void CALLBACK ExtractDone(ProgressContext* context, HANDLE screen)
{
	FarSInfo.RestoreScreen(screen);
}

static int ExtractError(int errorReason, HANDLE context)
{
	ProgressContext* pctx = (ProgressContext*) context;
	
	static const char* InfoLines[7];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_FAILED);
	InfoLines[2] = pctx->szFilePath;
	InfoLines[3] = "Retry";
	InfoLines[4] = "Skip";
	InfoLines[5] = "Skip All";
	InfoLines[6] = "Abort";

	int nMsg = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_WARNING, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 4);

	switch (nMsg)
	{
		case 0:
			return EEN_RETRY;
		case 1:
			return EEN_SKIP;
		case 2:
			return EEN_SKIPALL;
		case 3:
			return EEN_ABORT;
	}

	return EEN_SKIP;
}
//-----------------------------------  Extract functions ----------------------------------------

// Overwrite values
#define EXTR_OVERWRITE_ASK 1
#define EXTR_OVERWRITE_SILENT 2
#define EXTR_OVERWRITE_SKIP 3
#define EXTR_OVERWRITE_SKIPSILENT 4

static bool AskExtractOverwrite(int &overwrite, WIN32_FIND_DATAW existingFile, WIN32_FIND_DATAW newFile)
{
	__int64 nOldSize = ((__int64) existingFile.nFileSizeHigh >> 32) + existingFile.nFileSizeLow;
	__int64 nNewSize = ((__int64) newFile.nFileSizeHigh >> 32) + newFile.nFileSizeLow;
	
	SYSTEMTIME stOldUTC, stOldLocal;
	FileTimeToSystemTime(&existingFile.ftLastWriteTime, &stOldUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stOldUTC, &stOldLocal);

	SYSTEMTIME stNewUTC, stNewLocal;
	FileTimeToSystemTime(&newFile.ftLastWriteTime, &stNewUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stNewUTC, &stNewLocal);

	char szFileName[MAX_PATH] = {0};
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, newFile.cFileName, wcslen(newFile.cFileName), szFileName, MAX_PATH, NULL, NULL);
	
	static char szDialogLine1[120] = {0};
	sprintf_s(szDialogLine1, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), GetLocMsg(MSG_EXTRACT_OVERWRITE), szFileName);
	static char szDialogLine2[120] = {0};
	sprintf_s(szDialogLine2, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), "%21s  %s", "Size", "Last Modification");
	static char szDialogLine3[120] = {0};
	sprintf_s(szDialogLine3, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), "Old file: %11u  %5u-%02u-%02u %02u:%02u",
		(UINT) nOldSize, stOldLocal.wYear, stOldLocal.wMonth, stOldLocal.wDay, stOldLocal.wHour, stOldLocal.wMinute);
	static char szDialogLine4[120] = {0};
	sprintf_s(szDialogLine4, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), "New file: %11u  %5u-%02u-%02u %02u:%02u",
		(UINT) nNewSize, stNewLocal.wYear, stNewLocal.wMonth, stNewLocal.wDay, stNewLocal.wHour, stNewLocal.wMinute);
	
	static const char* DlgLines[10];
	DlgLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	DlgLines[1] = szDialogLine1;
	DlgLines[2] = szDialogLine2;
	DlgLines[3] = szDialogLine3;
	DlgLines[4] = szDialogLine4;
	DlgLines[5] = GetLocMsg(MSG_OVERWRITE);
	DlgLines[6] = GetLocMsg(MSG_OVERWRITE_ALL);
	DlgLines[7] = GetLocMsg(MSG_SKIP);
	DlgLines[8] = GetLocMsg(MSG_SKIP_ALL);
	DlgLines[9] = GetLocMsg(MSG_CANCEL);

	int nMsg = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_WARNING, NULL, DlgLines, sizeof(DlgLines) / sizeof(DlgLines[0]), 5);
	
	if ((nMsg == 4) // Cancel is pressed
		|| (nMsg == -1)) //Escape is pressed
		return false;
	else
	{
		overwrite = nMsg + 1;
		return true;
	}
}

static int ExtractStorageItem(StorageObject* storage, ContentTreeNode* item, const wchar_t* destDir, bool silent, int &doOverwrite, bool &skipOnError, HANDLE callbackContext)
{
	if (!item || !storage || item->IsDir()) return FALSE;

	// Check for ESC pressed
	if (CheckEsc())	return FALSE;

	static wchar_t wszItemSubPath[PATH_BUFFER_SIZE];
	item->GetPath(wszItemSubPath, PATH_BUFFER_SIZE, storage->CurrentDir());

	wstring strFullTargetPath(destDir);
	strFullTargetPath.append(wszItemSubPath);

	// Ask about overwrite if needed
	WIN32_FIND_DATAW fdExistingFile = {0};
	if (!silent && FileExists(strFullTargetPath.c_str(), &fdExistingFile))
	{
		if (doOverwrite == EXTR_OVERWRITE_ASK)
			if (!AskExtractOverwrite(doOverwrite, fdExistingFile, item->data))
				return FALSE;
		
		// Check either ask result or present value
		if (doOverwrite == EXTR_OVERWRITE_SKIP)
		{
			doOverwrite = EXTR_OVERWRITE_ASK;
			return TRUE;
		}
		else if (doOverwrite == EXTR_OVERWRITE_SKIPSILENT)
		{
			return TRUE;
		}
	}

	// Strip target path from file name
	wstring strTargetDir;
	size_t nLastSlash = strFullTargetPath.find_last_of('\\');
	if (nLastSlash != wstring::npos)
	{
		// Copy directory name without trailing backslash or following existence checker won't work
		strTargetDir = strFullTargetPath.substr(0, nLastSlash);
	}

	// Create target directory if needed
	if (strTargetDir.length() > 0)
	{
		if (!ForceDirectoryExist(strTargetDir.c_str()))
			return FALSE;
		strTargetDir.append(L"\\");
	}

	ProgressContext* pctx = (ProgressContext*) callbackContext;
	HANDLE hScreen;

	int ret;
	do
	{
		// Set extract params
		ExtractOperationParams params;
		params.item = item->storageIndex;
		params.flags = 0;
		params.destFilePath = strFullTargetPath.c_str();
		params.callbacks.FileProgress = ExtractProgress;
		params.callbacks.signalContext = callbackContext;

		ExtractStart(item, pctx, hScreen);
		ret = storage->Extract(params);
		ExtractDone(pctx, hScreen);

		if ((ret == SER_ERROR_WRITE) || (ret == SER_ERROR_READ))
		{
			int errorResp = EEN_ABORT;
			if (skipOnError)
				errorResp = EEN_SKIP;
			else if (!silent)
				errorResp = ExtractError(ret, callbackContext);

			switch (errorResp)
			{
				case EEN_ABORT:
					ret = SER_USERABORT;
					break;
				case EEN_SKIP:
					ret = SER_SUCCESS;
					break;
				case EEN_SKIPALL:
					ret = SER_SUCCESS;
					skipOnError = true;
					break;
			}
		}
		else if (ret == SER_ERROR_SYSTEM)
		{
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_FAILED, pctx->szFilePath);
		}

	} while ((ret != SER_SUCCESS) && (ret != SER_ERROR_SYSTEM) && (ret != SER_USERABORT));

	return (ret == SER_SUCCESS);
}

bool ConfirmExtract(int NumFiles, int NumDirectories, const char *DestPath)
{
	static char szDialogLine1[120] = {0};
	sprintf_s(szDialogLine1, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), GetLocMsg(MSG_EXTRACT_CONFIRM), NumFiles, NumDirectories);

	static const char* ConfirmBox[3];
	ConfirmBox[0] = GetLocMsg(MSG_PLUGIN_NAME);
	ConfirmBox[1] = szDialogLine1;
	ConfirmBox[2] = DestPath;
	int choise = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_MB_OKCANCEL, NULL, ConfirmBox, 3, 2);

	return (choise == 0);
}

//-----------------------------------  Export functions ----------------------------------------

int WINAPI GetMinFarVersion(void)
{
	return FARMANAGERVERSION;
}

void WINAPI SetStartupInfo(const struct PluginStartupInfo *Info)
{
	FarSInfo = *Info;
	FSF = *Info->FSF;
	FarSInfo.FSF = &FSF;

	if (GetModuleFileNameW(g_hDllHandle, wszPluginLocation, MAX_PATH))
	{
		wchar_t *slash = wcsrchr(wszPluginLocation, '\\');
		if (slash)
			*(slash + 1) = 0;
	}
	else
	{
		wmemset(wszPluginLocation, 0, MAX_PATH);
	}

	LoadSettings();
	g_pController.Init(wszPluginLocation);
}

void WINAPI GetPluginInfo(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(PluginInfo);
	Info->Flags = 0;

	static const char *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetLocMsg(MSG_PLUGIN_NAME);
	
	static const char *ConfigMenuStrings[1];
	ConfigMenuStrings[0] = GetLocMsg(MSG_PLUGIN_CONFIG_NAME);

	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);
	Info->PluginConfigStrings = ConfigMenuStrings;
	Info->PluginConfigStringsNumber = sizeof(ConfigMenuStrings) / sizeof(ConfigMenuStrings[0]);
	Info->CommandPrefix = optPrefix;
}

void WINAPI ExitFAR(void)
{
	g_pController.Cleanup();
}

int WINAPI Configure(int ItemNumber)
{
	InitDialogItem InitItems []={
		DI_DOUBLEBOX, 3,1, 36, 7, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_TITLE),
		DI_CHECKBOX,  5,2,  0, 2, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_ENABLE),
		DI_CHECKBOX,  5,3,  0, 2, 0, 0, 0,0, (char*) GetLocMsg(MSG_CONFIG_PREFIX),
		DI_FIXEDIT,	  7,4, 17, 3, 0, 0, 0,0, "",
		DI_TEXT,	  5,5,  0, 6, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
		DI_BUTTON,	  0,6,  0, 7, 1, 0, DIF_CENTERGROUP,1,"OK",
		DI_BUTTON,    0,6,  0, 7, 0, 0, DIF_CENTERGROUP,0,(char*) GetLocMsg(MSG_CANCEL)
	};
	FarDialogItem DialogItems[sizeof(InitItems)/sizeof(InitItems[0])];

	InitDialogItems(InitItems,DialogItems, sizeof(InitItems)/sizeof(InitItems[0]));

	DialogItems[1].Selected = optEnabled;
	DialogItems[2].Selected = optUsePrefix;
	strcpy_s(DialogItems[3].Data, sizeof(DialogItems[3].Data) / sizeof(DialogItems[3].Data[0]), optPrefix);

	int ExitCode = FarSInfo.Dialog(FarSInfo.ModuleNumber, -1, -1, 40, 9, "ObserverConfig", DialogItems, sizeof(DialogItems)/sizeof(DialogItems[0]));

	if ((ExitCode == 5) && (strlen(DialogItems[3].Data) < MAX_PREFIX_SIZE))
	{
		optEnabled = DialogItems[1].Selected != 0;
		optUsePrefix = DialogItems[2].Selected != 0;
		strcpy_s(optPrefix, MAX_PREFIX_SIZE, DialogItems[3].Data);

		SaveSettings();
	
		return TRUE;
	}

	return FALSE;
}

void WINAPI ClosePlugin(HANDLE hPlugin)
{
	if (hPlugin != NULL)
		CloseStorage(hPlugin);
}

HANDLE WINAPI OpenPlugin(int OpenFrom, INT_PTR Item)
{
	// Unload plug-in if no submodules loaded
	if (g_pController.NumModules() == 0)
		return 0;
	
	char* szFullNameBuffer = new char[PATH_BUFFER_SIZE];
	char* szContainerSubpath = NULL;
	DWORD fpres = 0;
	
	if ((OpenFrom == OPEN_COMMANDLINE) && optUsePrefix)
	{
		char* szLocalNameBuffer = new char[PATH_BUFFER_SIZE];

		strcpy_s(szLocalNameBuffer, MAX_PATH, (char *) Item);
		FSF.Unquote(szLocalNameBuffer);
		FSF.ExpandEnvironmentStr(szLocalNameBuffer, szLocalNameBuffer, PATH_BUFFER_SIZE);
		fpres = GetFullPathNameA(szLocalNameBuffer, PATH_BUFFER_SIZE, szFullNameBuffer, NULL);

		char* szColonPos = strrchr(szFullNameBuffer, ':');
		if (szColonPos != NULL && (szColonPos - szFullNameBuffer) > 2)
		{
			*szColonPos = 0;
			szContainerSubpath = szColonPos + 1;
		}

		delete [] szLocalNameBuffer;
	}
	else if (OpenFrom == OPEN_PLUGINSMENU)
	{
		PanelInfo pi;
		memset(&pi, 0, sizeof(pi));
		if (FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi))
			if ((pi.SelectedItemsNumber == 1) && (pi.PanelType == PTYPE_FILEPANEL))
			{
				strcpy_s(szFullNameBuffer, PATH_BUFFER_SIZE, pi.CurDir);
				if (szFullNameBuffer[strlen(szFullNameBuffer) - 1] != '\\')
					strcat_s(szFullNameBuffer, PATH_BUFFER_SIZE, "\\");
				strcat_s(szFullNameBuffer, PATH_BUFFER_SIZE, pi.SelectedItems[0].FindData.cFileName);

				fpres = strlen(szFullNameBuffer);
			}
	}

	HANDLE hOpenResult = (fpres && (fpres < PATH_BUFFER_SIZE)) ? OpenStorage(szFullNameBuffer) : INVALID_HANDLE_VALUE;

	if (szContainerSubpath != NULL && hOpenResult != INVALID_HANDLE_VALUE)
		SetDirectory(hOpenResult, szContainerSubpath, 0);

	delete [] szFullNameBuffer;
	return hOpenResult;
}

HANDLE WINAPI OpenFilePlugin(char *Name, const unsigned char *Data, int DataSize)
{
	if (!Name || !optEnabled)
		return INVALID_HANDLE_VALUE;

	HANDLE hOpenResult = OpenStorage(Name);
	return hOpenResult;
}

int WINAPI GetFindData(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	StorageObject* info = (StorageObject *) hPlugin;
	if (!info || !info->CurrentDir()) return FALSE;

	int nTotalItems = info->CurrentDir()->GetChildCount();
	*pItemsNumber = nTotalItems;

	// Zero items - exit now
	if (nTotalItems == 0) return TRUE;

	*pPanelItem = (PluginPanelItem *) malloc(nTotalItems * sizeof(PluginPanelItem));
	PluginPanelItem *panelItem = *pPanelItem;

	// Display all directories
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->subdirs.begin(); cit != info->CurrentDir()->subdirs.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		WIN32_FIND_DATAW* fd = &(cit->second->data);

		WideCharToMultiByte(CP_FAR_INTERNAL, 0, fd->cFileName, wcslen(fd->cFileName), panelItem->FindData.cFileName, MAX_PATH, NULL, NULL);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, fd->cAlternateFileName, wcslen(fd->cAlternateFileName), panelItem->FindData.cAlternateFileName, 14, NULL, NULL);
		panelItem->FindData.dwFileAttributes = fd->dwFileAttributes;
		panelItem->FindData.ftCreationTime = fd->ftCreationTime;
		panelItem->FindData.ftLastWriteTime = fd->ftLastWriteTime;
		panelItem->FindData.nFileSizeHigh = fd->nFileSizeHigh;
		panelItem->FindData.nFileSizeLow = fd->nFileSizeLow;

		panelItem++;
	}

	// Display all files
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->files.begin(); cit != info->CurrentDir()->files.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		WIN32_FIND_DATAW* fd = &(cit->second->data);

		WideCharToMultiByte(CP_FAR_INTERNAL, 0, fd->cFileName, wcslen(fd->cFileName), panelItem->FindData.cFileName, MAX_PATH, NULL, NULL);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, fd->cAlternateFileName, wcslen(fd->cAlternateFileName), panelItem->FindData.cAlternateFileName, 14, NULL, NULL);
		panelItem->FindData.dwFileAttributes = fd->dwFileAttributes;
		panelItem->FindData.ftCreationTime = fd->ftCreationTime;
		panelItem->FindData.ftLastWriteTime = fd->ftLastWriteTime;
		panelItem->FindData.ftLastAccessTime = fd->ftLastAccessTime;
		panelItem->FindData.nFileSizeHigh = fd->nFileSizeHigh;
		panelItem->FindData.nFileSizeLow = fd->nFileSizeLow;

		panelItem++;
	}

	return TRUE;
}

void WINAPI FreeFindData(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	free(PanelItem);
}

int WINAPI SetDirectory(HANDLE hPlugin, const char *Dir, int OpMode)
{
	if (hPlugin == NULL || hPlugin == INVALID_HANDLE_VALUE)
		return FALSE;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return FALSE;

	if (!Dir || !*Dir) return TRUE;

	wchar_t wzDirName[MAX_PATH] = {0};
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, Dir, strlen(Dir), wzDirName, MAX_PATH);
	return info->ChangeCurrentDir(wzDirName);
}

enum InfoLines
{
	IL_FORMAT = 1,
	IL_SIZE = 2,
	IL_FILES = 3,
	IL_DIRECTORIES = 4,
	IL_COMPRESS = 5,
	IL_COMMENT = 6,
	IL_CREATED = 7
};

void WINAPI GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
	Info->StructSize = sizeof(OpenPluginInfo);
	
	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return;
	
	static char szCurrentDir[MAX_PATH];
	static char szTitle[512];
	static wchar_t wszCurrentDirPath[PATH_BUFFER_SIZE];
	static char szHostFile[MAX_PATH];
	static KeyBarTitles KeyBar;

	memset(szCurrentDir, 0, sizeof(szCurrentDir));
	memset(szTitle, 0, sizeof(szTitle));
	memset(wszCurrentDirPath, 0, sizeof(wszCurrentDirPath));
	memset(szHostFile, 0, sizeof(szHostFile));

	info->CurrentDir()->GetPath(wszCurrentDirPath, PATH_BUFFER_SIZE);
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, wszCurrentDirPath, wcslen(wszCurrentDirPath), szCurrentDir, MAX_PATH, NULL, NULL);

	char szModuleName[32] = {0};
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GetModuleName(), -1, szModuleName, ARRAY_SIZE(szModuleName), NULL, NULL);
	sprintf_s(szTitle, ARRAY_SIZE(szTitle), "%s%s:\\%s", optPanelHeaderPrefix, szModuleName, szCurrentDir);

	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->StoragePath(), -1, szHostFile, MAX_PATH, NULL, NULL);

	Info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS;
	Info->CurDir = szCurrentDir;
	Info->PanelTitle = szTitle;
	Info->HostFile = szHostFile;

	// Fill info lines
	static InfoPanelLine pInfoLinesData[8];
	size_t nInfoTextSize = sizeof(pInfoLinesData[0].Text) / sizeof(pInfoLinesData[0].Text[0]);
	size_t nInfoDataSize = sizeof(pInfoLinesData[0].Data) / sizeof(pInfoLinesData[0].Data[0]);

	char *szHostFileName = strrchr(szHostFile, '\\');
	if (szHostFileName) szHostFileName++; else szHostFileName = szHostFile;
	
	memset(pInfoLinesData, 0, sizeof(pInfoLinesData));
	strncpy_s(pInfoLinesData[0].Text, nInfoTextSize, szHostFileName, nInfoTextSize-1);
	pInfoLinesData[0].Separator = 1;
	
	strcpy_s(pInfoLinesData[IL_FORMAT].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_FORMAT));
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GeneralInfo.Format, wcslen(info->GeneralInfo.Format), pInfoLinesData[IL_FORMAT].Data, nInfoDataSize, NULL, NULL);

	strcpy_s(pInfoLinesData[IL_SIZE].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_SIZE));
	_i64toa_s(info->TotalSize(), pInfoLinesData[IL_SIZE].Data, nInfoDataSize, 10);
	InsertCommas(pInfoLinesData[IL_SIZE].Data);
	
	strcpy_s(pInfoLinesData[IL_FILES].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_FILES));
	_ultoa_s(info->NumFiles(), pInfoLinesData[IL_FILES].Data, nInfoDataSize, 10);

	strcpy_s(pInfoLinesData[IL_DIRECTORIES].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_DIRS));
	_ultoa_s(info->NumDirectories(), pInfoLinesData[IL_DIRECTORIES].Data, nInfoDataSize, 10);

	strcpy_s(pInfoLinesData[IL_COMPRESS].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_COMPRESSION));
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GeneralInfo.Compression, wcslen(info->GeneralInfo.Compression), pInfoLinesData[IL_COMPRESS].Data, nInfoDataSize, NULL, NULL);
	pInfoLinesData[IL_COMPRESS].Data[STORAGE_PARAM_MAX_LEN] = 0;

	strcpy_s(pInfoLinesData[IL_COMMENT].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_COMMENT));
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->GeneralInfo.Comment, wcslen(info->GeneralInfo.Comment), pInfoLinesData[IL_COMMENT].Data, nInfoDataSize, NULL, NULL);
	pInfoLinesData[IL_COMMENT].Data[STORAGE_PARAM_MAX_LEN] = 0;

	strcpy_s(pInfoLinesData[IL_CREATED].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_CREATED));
	SYSTEMTIME st;
	if (info->GeneralInfo.Created.dwHighDateTime && FileTimeToSystemTime(&info->GeneralInfo.Created, &st))
		sprintf_s(pInfoLinesData[IL_CREATED].Data, nInfoDataSize, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	else
		strcpy_s(pInfoLinesData[IL_CREATED].Data, nInfoDataSize, "-");
	
	Info->InfoLinesNumber = sizeof(pInfoLinesData) / sizeof(pInfoLinesData[0]);
	Info->InfoLines = pInfoLinesData;

	memset(&KeyBar, 0, sizeof(KeyBar));
	KeyBar.ShiftTitles[0] = "";
	KeyBar.AltTitles[6-1] = (char*) GetLocMsg(MSG_ALTF6);
	Info->KeyBar = &KeyBar;
}

int WINAPI GetFiles(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	if (Move || !DestPath || (OpMode & OPM_FIND) || !ItemsNumber)
		return 0;

	// Check for single '..' item, do not show confirm dialog
	if ((ItemsNumber == 1) && (strcmp(PanelItem[0].FindData.cFileName, "..") == 0))
		return 0;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return 0;

	vector<int> vcExtractItems;
	__int64 nTotalExtractSize = 0;
	wchar_t wszItemNameBuf[MAX_PATH] = {0};
	int nExtNumFiles = 0, nExtNumDirs = 0;

	// Collect all indices of items to extract
	for (int i = 0; i < ItemsNumber; i++)
	{
		PluginPanelItem pItem = PanelItem[i];
		if (strcmp(pItem.FindData.cFileName, "..") == 0) continue;

		wmemset(wszItemNameBuf, 0, MAX_PATH);
		MultiByteToWideChar(CP_FAR_INTERNAL, 0, pItem.FindData.cFileName, strlen(pItem.FindData.cFileName), wszItemNameBuf, MAX_PATH);

		ContentTreeNode* child = info->CurrentDir()->GetChildByName(wszItemNameBuf);
		if (child) CollectFileList(child, vcExtractItems, nTotalExtractSize, true);

		if (pItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			nExtNumDirs++;
		else
			nExtNumFiles++;
	} //for

	// Check if we have something to extract
	if (vcExtractItems.size() == 0)
		return 0;

	// Confirm extraction
	if ((OpMode & OPM_SILENT) == 0)
	{
		if (!ConfirmExtract(nExtNumFiles, nExtNumDirs, DestPath))
			return -1;
	}

	// Items should be sorted (e.g. for access to solid archives)
	sort(vcExtractItems.begin(), vcExtractItems.end());

	// Prepare destination path
	size_t nDestPathLen = strlen(DestPath);
	wchar_t *wszWideDestPath = (wchar_t *) malloc((nDestPathLen + 2) * sizeof(wchar_t));
	wmemset(wszWideDestPath, 0, nDestPathLen + 2);
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, DestPath, nDestPathLen, wszWideDestPath, nDestPathLen + 2);
	if (wszWideDestPath[nDestPathLen - 1] != '\\')
		wcscat_s(wszWideDestPath, nDestPathLen + 2, L"\\");

	if (!ForceDirectoryExist(wszWideDestPath))
	{
		if ((OpMode & OPM_SILENT) == 0)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, NULL);
		return 0;
	}

	if (!IsEnoughSpaceInPath(wszWideDestPath, nTotalExtractSize))
	{
		DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_NODISKSPACE, NULL);
		return 0;
	}

	int nExtractResult = TRUE;
	int doOverwrite = EXTR_OVERWRITE_ASK;
	bool skipOnError = false;

	ProgressContext pctx;
	pctx.nTotalFiles = vcExtractItems.size();
	pctx.nTotalSize = nTotalExtractSize;

	// Extract all files one by one
	for (vector<int>::const_iterator cit = vcExtractItems.begin(); cit != vcExtractItems.end(); cit++)
	{
		ContentTreeNode* nextItem = info->GetItem(*cit);

		nExtractResult = ExtractStorageItem(info, nextItem, wszWideDestPath, (OpMode & OPM_SILENT) > 0, doOverwrite, skipOnError, &pctx);

		if (!nExtractResult) break;
	}

	free(wszWideDestPath);

	//TODO: add recursion option
	//TODO: add option to keep full path
	return nExtractResult;
}

int WINAPI ProcessKey(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	if (Key == VK_F6 && ControlState == PKF_ALT)
	{
		StorageObject* info = (StorageObject *) hPlugin;
		if (!info) return FALSE;
		
		PanelInfo pi = {0};
		if (!FarSInfo.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi)) return FALSE;

		size_t nPathSize = wcslen(info->StoragePath());
		char* szTargetDir = (char *) malloc(nPathSize + 1);
		memset(szTargetDir, 0, nPathSize + 1);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, info->StoragePath(), nPathSize, szTargetDir, nPathSize + 1, NULL, NULL);
		
		char* szLastSlash = strrchr(szTargetDir, '\\');
		if (szLastSlash) *(szLastSlash + 1) = 0;

		GetFiles(hPlugin, pi.SelectedItems, pi.SelectedItemsNumber, 0, szTargetDir, OPM_SILENT);

		free(szTargetDir);
		
		return TRUE;
	}

	return FALSE;
}
