// Observer.cpp : Defines the exported functions for the DLL application.
// This module contains functions for ANSI version of FAR (1.75+)

#include "StdAfx.h"
#include <far2/plugin.hpp>

#include "CommonFunc.h"
#include "ModulesController.h"
#include "PlugLang.h"
#include "FarStorage.h"

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
static wchar_t optPrefix[MAX_PREFIX_SIZE] = L"observe";

//-----------------------------------  Local functions ----------------------------------------

static const wchar_t* GetLocMsg(int MsgID)
{
	return FarSInfo.GetMsg(FarSInfo.ModuleNumber, MsgID);
}

static void LoadSettings()
{
	wstring strOptKey(FarSInfo.RootKey);
	strOptKey.append(L"\\Observer");

	HKEY reg;
	if (RegOpenKey(HKEY_CURRENT_USER, strOptKey.c_str(), &reg) != ERROR_SUCCESS) return;

	DWORD dwValueSize;
	RegQueryValueEx(reg, L"Enabled", NULL, NULL, (LPBYTE) &optEnabled, &(dwValueSize = sizeof(optEnabled)));
	RegQueryValueEx(reg, L"UsePrefix", NULL, NULL, (LPBYTE) &optUsePrefix, &(dwValueSize = sizeof(optPrefix)));
	RegQueryValueEx(reg, L"Prefix", NULL, NULL, (LPBYTE) &optPrefix, &(dwValueSize = MAX_PREFIX_SIZE));

	RegCloseKey(reg);
}

static void SaveSettings()
{
	wstring strOptKey(FarSInfo.RootKey);
	strOptKey.append(L"\\Observer");

	HKEY reg;
	if (RegCreateKey(HKEY_CURRENT_USER, strOptKey.c_str(), &reg) != ERROR_SUCCESS) return;

	RegSetValueEx(reg, L"Enabled", 0, REG_DWORD, (BYTE *) &optEnabled, sizeof(optEnabled));
	RegSetValueEx(reg, L"UsePrefix", 0, REG_DWORD, (BYTE *) &optUsePrefix, sizeof(optUsePrefix));
	RegSetValueEx(reg, L"Prefix", 0, REG_SZ, (BYTE *) &optPrefix, wcslen(optPrefix) + 1);
	
	RegCloseKey(reg);
}

static void InsertCommas(wchar_t *Dest)
{
  int I;
  for (I=wcslen(Dest)-4;I>=0;I-=3)
    if (Dest[I])
    {
      wmemmove(Dest+I+2,Dest+I+1,wcslen(Dest+I));
      Dest[I+1]=',';
    }
}

static void DisplayMessage(bool isError, bool isInteractive, int headerMsgID, int textMsgID, const wchar_t* errorItem)
{
	static const wchar_t* MsgLines[3];
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

static HANDLE OpenStorage(const wchar_t* Name)
{
	StorageObject *storage = new StorageObject(&g_pController);
	if (!storage->Open(Name))
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

	static wchar_t szFileProgressLine[100] = {0};
	swprintf_s(szFileProgressLine, 100, L"File: %d/%d. Progress: %2d%% / %2d%%", pc->nCurrentFileNumber, pc->nTotalFiles, pc->nCurrentFileProgress, nTotalProgress);

	static const wchar_t* InfoLines[4];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_EXTRACTING);
	InfoLines[2] = szFileProgressLine;
	InfoLines[3] = pc->wszFilePath;

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

	wmemset(context->wszFilePath, 0, MAX_PATH);
	wcscpy_s(context->wszFilePath, MAX_PATH, wszSubPath);
	
	// Shrink file path to fit on console
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hStdOut, &si))
		FSF.TruncPathStr(context->wszFilePath, si.dwSize.X - 16);

	return ExtractProgress((HANDLE)context);
}

static void CALLBACK ExtractDone(ProgressContext* context, HANDLE screen)
{
	FarSInfo.RestoreScreen(screen);
}

static int ExtractError(int errorReason, HANDLE context)
{
	ProgressContext* pctx = (ProgressContext*) context;
	
	static const wchar_t* InfoLines[6];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_FAILED);
	InfoLines[2] = pctx->wszFilePath;
	InfoLines[3] = L"Retry";
	InfoLines[4] = L"Skip";
	InfoLines[5] = L"Abort";

	int nMsg = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_WARNING, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 3);

	switch (nMsg)
	{
		case 0:
			return EEN_RETRY;
		case 1:
			return EEN_SKIP;
		case 2:
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
	
	static wchar_t szDialogLine1[120] = {0};
	swprintf_s(szDialogLine1, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), GetLocMsg(MSG_EXTRACT_OVERWRITE), newFile.cFileName);
	static wchar_t szDialogLine2[120] = {0};
	swprintf_s(szDialogLine2, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), L"%21s  %s", "Size", "Last Modification");
	static wchar_t szDialogLine3[120] = {0};
	swprintf_s(szDialogLine3, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), L"Old file: %11u  %5u-%02u-%02u %02u:%02u",
		(UINT) nOldSize, stOldLocal.wYear, stOldLocal.wMonth, stOldLocal.wDay, stOldLocal.wHour, stOldLocal.wMinute);
	static wchar_t szDialogLine4[120] = {0};
	swprintf_s(szDialogLine4, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), L"New file: %11u  %5u-%02u-%02u %02u:%02u",
		(UINT) nNewSize, stNewLocal.wYear, stNewLocal.wMonth, stNewLocal.wDay, stNewLocal.wHour, stNewLocal.wMinute);
	
	static const wchar_t* DlgLines[10];
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

static int ExtractStorageItem(StorageObject* storage, ContentTreeNode* item, const wchar_t* destDir, bool silent, int &doOverwrite, HANDLE callbackContext)
{
	if (!item || !storage || item->IsDir())
		return SER_ERROR_READ;

	// Check for ESC pressed
	if (CheckEsc())	return SER_USERABORT;

	static wchar_t wszItemSubPath[PATH_BUFFER_SIZE];
	item->GetPath(wszItemSubPath, PATH_BUFFER_SIZE, storage->CurrentDir());

	wstring strFullTargetPath(destDir);
	if (strFullTargetPath[strFullTargetPath.length() - 1] != '\\')
		strFullTargetPath.append(L"\\");
	strFullTargetPath.append(wszItemSubPath);

	// Ask about overwrite if needed
	WIN32_FIND_DATAW fdExistingFile = {0};
	if (!silent && FileExists(strFullTargetPath.c_str(), &fdExistingFile))
	{
		if (doOverwrite == EXTR_OVERWRITE_ASK)
			if (!AskExtractOverwrite(doOverwrite, fdExistingFile, item->data))
				return SER_USERABORT;
		
		// Check either ask result or present value
		if (doOverwrite == EXTR_OVERWRITE_SKIP)
		{
			doOverwrite = EXTR_OVERWRITE_ASK;
			return SER_SUCCESS;
		}
		else if (doOverwrite == EXTR_OVERWRITE_SKIPSILENT)
		{
			return SER_SUCCESS;
		}
	}

	// Strip target path from file name
	wstring strTargetDir;
	size_t nLastSlash = strFullTargetPath.find_last_of('\\');
	if (nLastSlash != wstring::npos)
		// Copy directory name without trailing backslash or following existence checker won't work
		strTargetDir = strFullTargetPath.substr(0, nLastSlash);

	// Create target directory if needed
	if (strTargetDir.length() > 0)
	{
		if (!ForceDirectoryExist(strTargetDir.c_str()))
			return SER_ERROR_WRITE;
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
		params.dest_path = strTargetDir.c_str();
		params.callbacks.FileProgress = ExtractProgress;
		params.callbacks.signalContext = callbackContext;

		ExtractStart(item, pctx, hScreen);
		ret = storage->Extract(params);
		ExtractDone(pctx, hScreen);

		if ((ret == SER_ERROR_WRITE) || (ret == SER_ERROR_READ))
		{
			int errorResp = silent ? EEN_ABORT : ExtractError(ret, callbackContext);
			switch (errorResp)
			{
				case EEN_ABORT:
					ret = SER_USERABORT;
					break;
				case EEN_SKIP:
					ret = SER_SUCCESS;
					break;
			}
		}
		else if (ret == SER_ERROR_SYSTEM)
		{
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_FAILED, pctx->wszFilePath);
		}

	} while ((ret != SER_SUCCESS) && (ret != SER_ERROR_SYSTEM) && (ret != SER_USERABORT));

	return ret;
}

int BatchExtract(StorageObject* info, vector<int> &items, __int64 totalExtractSize, const wchar_t* DestPath, bool silent)
{
	// Items should be sorted (e.g. for access to solid archives)
	sort(items.begin(), items.end());

	if (!ForceDirectoryExist(DestPath))
	{
		if (!silent)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, NULL);
		return 0;
	}

	if (!IsEnoughSpaceInPath(DestPath, totalExtractSize))
	{
		DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_NODISKSPACE, NULL);
		return 0;
	}

	int nExtractResult = TRUE;
	int doOverwrite = EXTR_OVERWRITE_ASK;

	ProgressContext pctx;
	pctx.nTotalFiles = items.size();
	pctx.nTotalSize = totalExtractSize;

	// Extract all files one by one
	for (vector<int>::const_iterator cit = items.begin(); cit != items.end(); cit++)
	{
		ContentTreeNode* nextItem = info->GetItem(*cit);

		nExtractResult = ExtractStorageItem(info, nextItem, DestPath, silent, doOverwrite, &pctx);

		if (nExtractResult != SER_SUCCESS) break;
	}

	if (nExtractResult == SER_USERABORT)
		return -1;
	else if (nExtractResult == SER_SUCCESS)
		return 1;

	return 0;
}

//-----------------------------------  Export functions ----------------------------------------

int WINAPI GetMinFarVersionW(void)
{
	return FARMANAGERVERSION;
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	FarSInfo = *Info;
	FSF = *Info->FSF;
	FarSInfo.FSF = &FSF;

	if (GetModuleFileName(g_hDllHandle, wszPluginLocation, MAX_PATH))
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

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(PluginInfo);
	Info->Flags = 0;

	static const wchar_t *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetLocMsg(MSG_PLUGIN_NAME);
	
	static const wchar_t *ConfigMenuStrings[1];
	ConfigMenuStrings[0] = GetLocMsg(MSG_PLUGIN_CONFIG_NAME);

	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);
	Info->PluginConfigStrings = ConfigMenuStrings;
	Info->PluginConfigStringsNumber = sizeof(ConfigMenuStrings) / sizeof(ConfigMenuStrings[0]);
	Info->CommandPrefix = optPrefix;
}

void WINAPI ExitFARW(void)
{
	g_pController.Cleanup();
}

int WINAPI ConfigureW(int ItemNumber)
{
	FarDialogItem DialogItems []={
		DI_DOUBLEBOX, 3,1, 35, 7, 0, 0, 0,0, GetLocMsg(MSG_CONFIG_TITLE), 0,
		DI_CHECKBOX,  5,2,  0, 2, 0, optEnabled, 0,0, GetLocMsg(MSG_CONFIG_ENABLE), 0,
		DI_CHECKBOX,  5,3,  0, 2, 0, optUsePrefix, 0,0, GetLocMsg(MSG_CONFIG_PREFIX), 0,
		DI_FIXEDIT,	  7,4, 17, 3, 0, 0, 0,0, optPrefix, 0,
		DI_TEXT,	  3,5,  0, 5, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, L"", 0,
		DI_BUTTON,	  0,6,  0, 7, 1, 0, DIF_CENTERGROUP, 1, L"OK", 0,
		DI_BUTTON,    0,6,  0, 7, 0, 0, DIF_CENTERGROUP, 0, L"Cancel", 0,
	};

	HANDLE hDlg = FarSInfo.DialogInit(FarSInfo.ModuleNumber, -1, -1, 38, 9, L"TempCfg",
		DialogItems, sizeof(DialogItems) / sizeof(DialogItems[0]), 0, 0, FarSInfo.DefDlgProc, 0);

	if (hDlg != INVALID_HANDLE_VALUE)
	{
		int ExitCode = FarSInfo.DialogRun(hDlg);
		if (ExitCode == 5) // OK was pressed
		{
			FarDialogItem *dlgItem;

			dlgItem = (FarDialogItem*) malloc(FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, 1, NULL));
			FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, 1, (LONG_PTR) dlgItem);
			optEnabled = dlgItem->Selected;
			free(dlgItem);

			dlgItem = (FarDialogItem*) malloc(FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, 2, NULL));
			FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, 2, (LONG_PTR) dlgItem);
			optUsePrefix = dlgItem->Selected;
			free(dlgItem);

			dlgItem = (FarDialogItem*) malloc(FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, 3, NULL));
			FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, 3, (LONG_PTR) dlgItem);
			wcscpy_s(optPrefix, sizeof(optPrefix) / sizeof(optPrefix[0]), dlgItem->PtrData);
			free(dlgItem);

			SaveSettings();
		}
		FarSInfo.DialogFree(hDlg);
		
		if (ExitCode == 5) return TRUE;
	}

	return FALSE;
}

void WINAPI ClosePluginW(HANDLE hPlugin)
{
	if (hPlugin != NULL)
		CloseStorage(hPlugin);
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	// Unload plug-in if no submodules loaded
	if (g_pController.NumModules() == 0)
		return 0;
	
	wchar_t* szFullNameBuffer = new wchar_t[PATH_BUFFER_SIZE];
	DWORD fpres = 0;
	
	if ((OpenFrom == OPEN_COMMANDLINE) && optUsePrefix)
	{
		wchar_t* szLocalNameBuffer = new wchar_t[PATH_BUFFER_SIZE];

		wcscpy_s(szLocalNameBuffer, MAX_PATH, (wchar_t *) Item);
		FSF.Unquote(szLocalNameBuffer);
		//FSF.ExpandEnvironmentStr(szLocalNameBuffer, szLocalNameBuffer, PATH_BUFFER_SIZE);
		fpres = GetFullPathName(szLocalNameBuffer, PATH_BUFFER_SIZE, szFullNameBuffer, NULL);

		delete [] szLocalNameBuffer;
	}
	else if (OpenFrom == OPEN_PLUGINSMENU)
	{
		PanelInfo pi = {0};
		if (FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi))
			if ((pi.SelectedItemsNumber == 1) && (pi.PanelType == PTYPE_FILEPANEL))
			{
				FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, PATH_BUFFER_SIZE, (LONG_PTR) szFullNameBuffer);
				if (szFullNameBuffer[wcslen(szFullNameBuffer) - 1] != '\\')
					wcscat_s(szFullNameBuffer, PATH_BUFFER_SIZE, L"\\");

				PluginPanelItem *PPI = (PluginPanelItem*)malloc(FarSInfo.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, NULL));
				if (PPI)
				{
					FarSInfo.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, (LONG_PTR)PPI);
					wcscat_s(szFullNameBuffer, PATH_BUFFER_SIZE, PPI->FindData.lpwszFileName);
					free(PPI);

					fpres = wcslen(szFullNameBuffer);
				}
			}
	}

	HANDLE hOpenResult = (fpres && (fpres < PATH_BUFFER_SIZE)) ? OpenStorage(szFullNameBuffer) : INVALID_HANDLE_VALUE;

	delete [] szFullNameBuffer;
	return hOpenResult;
}

HANDLE WINAPI OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	if (!Name || !optEnabled)
		return INVALID_HANDLE_VALUE;

	HANDLE hOpenResult = OpenStorage(Name);
	return hOpenResult;
}

int WINAPI GetFindDataW(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
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

		ContentTreeNode* node = (cit->second);

		panelItem->FindData.lpwszFileName = node->data.cFileName;
		panelItem->FindData.lpwszAlternateFileName = node->data.cAlternateFileName;
		panelItem->FindData.dwFileAttributes = node->data.dwFileAttributes;
		panelItem->FindData.ftCreationTime = node->data.ftCreationTime;
		panelItem->FindData.ftLastWriteTime = node->data.ftLastWriteTime;
		panelItem->FindData.ftLastAccessTime = node->data.ftLastAccessTime;
		panelItem->FindData.nFileSize = node->GetSize();

		panelItem++;
	}

	// Display all files
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->files.begin(); cit != info->CurrentDir()->files.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		ContentTreeNode* node = (cit->second);

		panelItem->FindData.lpwszFileName = node->data.cFileName;
		panelItem->FindData.lpwszAlternateFileName = node->data.cAlternateFileName;
		panelItem->FindData.dwFileAttributes = node->data.dwFileAttributes;
		panelItem->FindData.ftCreationTime = node->data.ftCreationTime;
		panelItem->FindData.ftLastWriteTime = node->data.ftLastWriteTime;
		panelItem->FindData.ftLastAccessTime = node->data.ftLastAccessTime;
		panelItem->FindData.nFileSize = node->GetSize();

		panelItem++;
	}

	return TRUE;
}

void WINAPI FreeFindDataW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	free(PanelItem);
}

int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
	if (hPlugin == NULL)
		return FALSE;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return FALSE;

	if (!Dir || !*Dir) return TRUE;

	return info->ChangeCurrentDir(Dir);
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

void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
	Info->StructSize = sizeof(OpenPluginInfo);
	
	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return;
	
	static wchar_t wszCurrentDir[MAX_PATH];
	static wchar_t wszTitle[512];
	static const wchar_t *wszHostFile = NULL;
	static KeyBarTitles KeyBar;
	
	static wchar_t wszStorageSizeInfo[32];
	static wchar_t wszNumFileInfo[16];
	static wchar_t wszNumDirsInfo[16];
	static wchar_t wszStorageCreatedInfo[32];

	memset(wszCurrentDir, 0, sizeof(wszCurrentDir));
	memset(wszTitle, 0, sizeof(wszTitle));

	wszCurrentDir[0] = '\\';
	info->CurrentDir()->GetPath(wszCurrentDir + 1, PATH_BUFFER_SIZE);
	swprintf_s(wszTitle, L"%s:%s", info->GetModuleName(), wszCurrentDir);

	// Ugly hack (FAR does not exit plug-in if root directory is \)
	if (wcslen(wszCurrentDir) == 1) wszCurrentDir[0] = 0;

	wszHostFile = wcsrchr(info->StoragePath(), '\\');
	if (wszHostFile) wszHostFile++; else wszHostFile = info->StoragePath();

	_i64tow_s(info->TotalSize(), wszStorageSizeInfo, sizeof(wszStorageSizeInfo)/sizeof(wszStorageSizeInfo[0]), 10);
	InsertCommas(wszStorageSizeInfo);

	_ultow_s(info->NumFiles(), wszNumFileInfo, sizeof(wszNumFileInfo)/sizeof(wszNumFileInfo[0]), 10);
	_ultow_s(info->NumDirectories(), wszNumDirsInfo, sizeof(wszNumDirsInfo)/sizeof(wszNumDirsInfo[0]), 10);

	SYSTEMTIME st;
	if (info->GeneralInfo.Created.dwHighDateTime && FileTimeToSystemTime(&info->GeneralInfo.Created, &st))
		swprintf_s(wszStorageCreatedInfo, 32, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	else
		wcscpy_s(wszStorageCreatedInfo, 32, L"-");
	
	// Fill info lines
	static InfoPanelLine pInfoLinesData[8];
	memset(pInfoLinesData, 0, sizeof(pInfoLinesData));

	pInfoLinesData[0].Data = wszHostFile;
	pInfoLinesData[0].Separator = 1;
	
	pInfoLinesData[IL_FORMAT].Text = GetLocMsg(MSG_INFOL_FORMAT);
	pInfoLinesData[IL_FORMAT].Data = info->GeneralInfo.Format;

	pInfoLinesData[IL_SIZE].Text = GetLocMsg(MSG_INFOL_SIZE);
	pInfoLinesData[IL_SIZE].Data = wszStorageSizeInfo;
	
	pInfoLinesData[IL_FILES].Text = GetLocMsg(MSG_INFOL_FILES);
	pInfoLinesData[IL_FILES].Data = wszNumFileInfo;
	
	pInfoLinesData[IL_DIRECTORIES].Text = GetLocMsg(MSG_INFOL_DIRS);
	pInfoLinesData[IL_DIRECTORIES].Data = wszNumDirsInfo;
	
	pInfoLinesData[IL_COMPRESS].Text = GetLocMsg(MSG_INFOL_COMPRESSION);
	pInfoLinesData[IL_COMPRESS].Data = info->GeneralInfo.Compression;

	pInfoLinesData[IL_COMMENT].Text = GetLocMsg(MSG_INFOL_COMMENT);
	pInfoLinesData[IL_COMMENT].Data = info->GeneralInfo.Comment;

	pInfoLinesData[IL_CREATED].Text = GetLocMsg(MSG_INFOL_CREATED);
	pInfoLinesData[IL_CREATED].Data = wszStorageCreatedInfo;
		
	// Fill report structure
	Info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS;
	Info->CurDir = wszCurrentDir;
	Info->PanelTitle = wszTitle;
	Info->HostFile = wszHostFile;
	Info->InfoLinesNumber = sizeof(pInfoLinesData) / sizeof(pInfoLinesData[0]);
	Info->InfoLines = pInfoLinesData;

	memset(&KeyBar, 0, sizeof(KeyBar));
	KeyBar.ShiftTitles[0] = L"";
	KeyBar.AltTitles[6-1] = (wchar_t*) GetLocMsg(MSG_ALTF6);
	Info->KeyBar = &KeyBar;
}

int WINAPI GetFilesW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode)
{
	if (Move || !DestPath || (OpMode & OPM_FIND) || !ItemsNumber)
		return 0;

	// Check for single '..' item, do not show confirm dialog
	if ((ItemsNumber == 1) && (wcscmp(PanelItem[0].FindData.lpwszFileName, L"..") == 0))
		return 0;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return 0;

	// Confirm extraction
	if ((OpMode & OPM_SILENT) == 0)
	{
		static const wchar_t* ConfirmBox[2];
		ConfirmBox[0] = GetLocMsg(MSG_PLUGIN_NAME);
		ConfirmBox[1] = GetLocMsg(MSG_EXTRACT_CONFIRM);
		int choise = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_MB_OKCANCEL, NULL, ConfirmBox, 2, 2);
		if (choise != 0) return -1;
	}

	vector<int> vcExtractItems;
	__int64 nTotalExtractSize = 0;

	// Collect all indices of items to extract
	for (int i = 0; i < ItemsNumber; i++)
	{
		PluginPanelItem pItem = PanelItem[i];
		if (wcscmp(pItem.FindData.lpwszFileName, L"..") == 0) continue;

		ContentTreeNode* child = info->CurrentDir()->GetChildByName(pItem.FindData.lpwszFileName);
		if (child)
		{
			CollectFileList(child, vcExtractItems, nTotalExtractSize, true);
		}
	} //for

	// Check if we have something to extract
	if (vcExtractItems.size() == 0)
		return 0;

	//TODO: add recursion option
	//TODO: add option to keep full path

	return BatchExtract(info, vcExtractItems, nTotalExtractSize, *DestPath, (OpMode & OPM_SILENT) > 0);
}

int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	if (Key == VK_F6 && ControlState == PKF_ALT)
	{
		StorageObject* info = (StorageObject *) hPlugin;
		if (!info) return FALSE;
		
		PanelInfo pi = {0};
		if (!FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR) &pi)) return FALSE;
		if (pi.SelectedItemsNumber == 0) return FALSE;

		vector<int> vcExtractItems;
		__int64 nTotalExtractSize = 0;

		// Collect all indices of items to extract
		for (int i = 0; i < pi.SelectedItemsNumber; i++)
		{
			PluginPanelItem* pItem = (PluginPanelItem*) malloc(FarSInfo.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, NULL));
			FarSInfo.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR) pItem);
			if (wcscmp(pItem->FindData.lpwszFileName, L"..") == 0) continue;

			ContentTreeNode* child = info->CurrentDir()->GetChildByName(pItem->FindData.lpwszFileName);
			if (child) CollectFileList(child, vcExtractItems, nTotalExtractSize, true);
			
			free(pItem);
		} //for

		// Check if we have something to extract
		if (vcExtractItems.size() == 0) return TRUE;

		wchar_t *wszTargetDir = _wcsdup(info->StoragePath());
		
		wchar_t* szLastSlash = wcsrchr(wszTargetDir, '\\');
		if (szLastSlash) *(szLastSlash + 1) = 0;

		BatchExtract(info, vcExtractItems, nTotalExtractSize, wszTargetDir, true);

		free(wszTargetDir);
		
		return TRUE;
	}

	return FALSE;
}