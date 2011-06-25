// Observer.cpp : Defines the exported functions for the DLL application.
// This module contains functions for ANSI version of FAR (1.75+)

#include "StdAfx.h"
#include <far2/plugin.hpp>

#include "ModulesController.h"
#include "PlugLang.h"
#include "FarStorage.h"
#include "CommonFunc.h"

#include "OptionsFile.h"
#include "RegistrySettings.h"

extern HMODULE g_hDllHandle;
static PluginStartupInfo FarSInfo;
static FarStandardFunctions FSF;

static wchar_t wszPluginLocation[MAX_PATH];
static wchar_t wszConfigFileLocation[MAX_PATH];
static ModulesController g_pController;

#define CP_FAR_INTERNAL CP_OEMCP
#define CONFIG_FILE L"observer.ini"

// Settings
#define MAX_PREFIX_SIZE 32
static int optEnabled = TRUE;
static int optUsePrefix = TRUE;
static int optUseExtensionFilters = TRUE;
static wchar_t optPrefix[MAX_PREFIX_SIZE] = L"observe";

// Extended settings
static wchar_t optPanelHeaderPrefix[MAX_PREFIX_SIZE] = L"";
static int optExtendedCurDir = FALSE;

struct ProgressContext
{
	wchar_t wszFilePath[MAX_PATH];
	int nCurrentFileNumber;
	__int64 nCurrentFileSize;
	
	int nTotalFiles;
	__int64 nTotalSize;

	__int64 nProcessedFileBytes;
	__int64 nTotalProcessedBytes;
	int nCurrentProgress;
	
	ProgressContext()
	{
		memset(wszFilePath, 0, sizeof(wszFilePath));
		nCurrentFileNumber = 0;
		nTotalFiles = 0;
		nTotalSize = 0;
		nProcessedFileBytes = 0;
		nCurrentFileSize = 0;
		nTotalProcessedBytes = 0;
		nCurrentProgress = -1;
	}
};

struct ExtractSelectedParams
{
	wstring strDestPath;
	int bRecursive;
	int nPathProcessing;			// from KeepPathValues
	int nOverwriteExistingFiles;    // 0 - skip, 1 - overwrite, 2 - ask

	ExtractSelectedParams(const wchar_t* dstPath)
	{
		strDestPath = dstPath;
		bRecursive = 1;
		nPathProcessing = KPV_PARTIAL;
		nOverwriteExistingFiles = 2;
	}
};

//-----------------------------------  Local functions ----------------------------------------

static const wchar_t* GetLocMsg(int MsgID)
{
	return FarSInfo.GetMsg(FarSInfo.ModuleNumber, MsgID);
}

static void LoadSettings()
{
	// Load static settings from .ini file.
	OptionsFile opFile(wszConfigFileLocation);
	OptionsList *opList = opFile.GetSection(L"General");
	if (opList != NULL)
	{
		opList->GetValue(L"PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
		opList->GetValue(L"ExtendedCurDir", optExtendedCurDir);
		opList->GetValue(L"UseExtensionFilters", optUseExtensionFilters);
		delete opList;
	}

	// Load dynamic settings from registry (they will overwrite static ones)
	RegistrySettings regOpts(FarSInfo.RootKey);
	if (regOpts.Open())
	{
		regOpts.GetValue(L"Enabled", optEnabled);
		regOpts.GetValue(L"UsePrefix", optUsePrefix);
		regOpts.GetValue(L"Prefix", optPrefix, MAX_PREFIX_SIZE);

		regOpts.GetValue(L"PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
		regOpts.GetValue(L"ExtendedCurDir", optExtendedCurDir);
		regOpts.GetValue(L"UseExtensionFilters", optUseExtensionFilters);
	}
}

static void SaveSettings()
{
	RegistrySettings regOpts(FarSInfo.RootKey);
	if (regOpts.Open(true))
	{
		regOpts.SetValue(L"Enabled", optEnabled);
		regOpts.SetValue(L"UsePrefix", optUsePrefix);
		regOpts.SetValue(L"Prefix", optPrefix);
		regOpts.SetValue(L"UseExtensionFilters", optUseExtensionFilters);
	}
}

static void InsertCommas(wchar_t *Dest)
{
  int I;
  for (I=(int)wcslen(Dest)-4;I>=0;I-=3)
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

static int DlgHlp_GetCheckBoxState(HANDLE hDlg, int ctrlIndex)
{
	FarDialogItem *dlgItem;
	int retVal;

	dlgItem = (FarDialogItem*) malloc(FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, ctrlIndex, NULL));
	FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, ctrlIndex, (LONG_PTR) dlgItem);
	retVal = dlgItem->Selected;
	free(dlgItem);

	return retVal;
}

static void DlgHlp_GetEditBoxText(HANDLE hDlg, int ctrlIndex, wstring &buf)
{
	FarDialogItem *dlgItem;

	dlgItem = (FarDialogItem*) malloc(FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, ctrlIndex, NULL));
	FarSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, ctrlIndex, (LONG_PTR) dlgItem);

	buf = dlgItem->PtrData;

	free(dlgItem);
}

static bool DlgHlp_GetEditBoxText(HANDLE hDlg, int ctrlIndex, wchar_t* buf, size_t bufSize)
{
	wstring tmpStr;
	DlgHlp_GetEditBoxText(hDlg, ctrlIndex, tmpStr);
	
	if (tmpStr.size() < bufSize)
	{
		wcscpy_s(buf, bufSize, tmpStr.c_str());
		return true;
	}

	return false;
}

static int SelectModuleToOpenFileAs()
{
	size_t nNumModules = g_pController.modules.size();
	
	FarMenuItem* MenuItems = new FarMenuItem[nNumModules];
	memset(MenuItems, 0, nNumModules * sizeof(FarMenuItem));
	for (size_t i = 0; i < nNumModules; i++)
	{
		const ExternalModule& modInfo = g_pController.modules[i];
		MenuItems[i].Text = modInfo.ModuleName;
	}

	int nMSel = FarSInfo.Menu(FarSInfo.ModuleNumber, -1, -1, 0, 0, GetLocMsg(MSG_OPEN_SELECT_MODULE), NULL, NULL, NULL, NULL, MenuItems, (int) nNumModules);

	delete [] MenuItems;
	return nMSel;
}

static bool StoragePasswordQuery(char* buffer, size_t bufferSize)
{
	wchar_t passBuf[100] = {0};

	bool fRet = FarSInfo.InputBox(GetLocMsg(MSG_PLUGIN_NAME), GetLocMsg(MSG_OPEN_PASS_REQUIRED), NULL, NULL, passBuf, ARRAY_SIZE(passBuf)-1, NULL, FIB_PASSWORD | FIB_NOUSELASTHISTORY) == TRUE;
	if (fRet)
	{
		memset(buffer, 0, bufferSize);
		WideCharToMultiByte(CP_ACP, 0, passBuf, -1, buffer, (int) bufferSize - 1, NULL, NULL);
	}
	return fRet;
}

//-----------------------------------  Content functions ----------------------------------------

static HANDLE OpenStorage(const wchar_t* Name, bool applyExtFilters, int moduleIndex)
{
	StorageObject *storage = new StorageObject(&g_pController, StoragePasswordQuery);
	if (storage->Open(Name, applyExtFilters, moduleIndex) != SOR_SUCCESS)
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

static bool GetSelectedPanelFilePath(wstring& nameStr)
{
	nameStr.clear();
	
	PanelInfo pi = {0};
	if (FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi))
		if ((pi.SelectedItemsNumber == 1) && (pi.PanelType == PTYPE_FILEPANEL))
		{
			wchar_t szNameBuffer[PATH_BUFFER_SIZE] = {0};
			FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, ARRAY_SIZE(szNameBuffer), (LONG_PTR) szNameBuffer);
			IncludeTrailingPathDelim(szNameBuffer, ARRAY_SIZE(szNameBuffer));

			PluginPanelItem *PPI = (PluginPanelItem*)malloc(FarSInfo.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, NULL));
			if (PPI)
			{
				FarSInfo.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, (LONG_PTR)PPI);
				if ((PPI->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					wcscat_s(szNameBuffer, ARRAY_SIZE(szNameBuffer), PPI->FindData.lpwszFileName);
					nameStr = szNameBuffer;
				}
				free(PPI);
			}
		}
	
	return (nameStr.size() > 0);
}

//-----------------------------------  Callback functions ----------------------------------------

static int CALLBACK ExtractProgress(HANDLE context, __int64 ProcessedBytes)
{
	// Check for ESC pressed
	if (CheckEsc())
		return FALSE;

	ProgressContext* pc = (ProgressContext*) context;
	pc->nProcessedFileBytes += ProcessedBytes;

	int nFileProgress = (pc->nCurrentFileSize > 0) ? (int) ((pc->nProcessedFileBytes * 100) / pc->nCurrentFileSize) : 0;
	int nTotalProgress = (pc->nTotalSize > 0) ? (int) (((pc->nTotalProcessedBytes + pc->nProcessedFileBytes) * 100) / pc->nTotalSize) : 0;

	if (nFileProgress != pc->nCurrentProgress)
	{
		pc->nCurrentProgress = nFileProgress;

		static wchar_t szFileProgressLine[100] = {0};
		swprintf_s(szFileProgressLine, 100, L"File: %d/%d. Progress: %2d%% / %2d%%", pc->nCurrentFileNumber, pc->nTotalFiles, nFileProgress, nTotalProgress);

		static const wchar_t* InfoLines[4];
		InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
		InfoLines[1] = GetLocMsg(MSG_EXTRACT_EXTRACTING);
		InfoLines[2] = szFileProgressLine;
		InfoLines[3] = pc->wszFilePath;

		FarSInfo.Message(FarSInfo.ModuleNumber, 0, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 0);

		// Win7 only feature
		if (pc->nTotalSize > 0)
		{
			PROGRESSVALUE pv;
			pv.Completed = pc->nTotalProcessedBytes + pc->nProcessedFileBytes;
			pv.Total = pc->nTotalSize;
			FarSInfo.AdvControl(FarSInfo.ModuleNumber, ACTL_SETPROGRESSVALUE, &pv);
		}
	}

	return TRUE;
}

static int CALLBACK ExtractStart(const ContentTreeNode* item, ProgressContext* context, HANDLE &screen)
{
	screen = FarSInfo.SaveScreen(0, 0, -1, -1);

	context->nCurrentFileNumber++;
	context->nCurrentFileSize = item->Size();
	context->nProcessedFileBytes = 0;
	context->nCurrentProgress = -1;
	
	wchar_t wszSubPath[PATH_BUFFER_SIZE] = {0};
	item->GetPath(wszSubPath, PATH_BUFFER_SIZE);
		
	// Save file name for dialogs
	size_t nPathLen = wcslen(wszSubPath);
	wchar_t* wszSubPathPtr = wszSubPath;
	if (nPathLen > MAX_PATH) wszSubPathPtr += (nPathLen % MAX_PATH);

	wmemset(context->wszFilePath, 0, MAX_PATH);
	wcscpy_s(context->wszFilePath, MAX_PATH, wszSubPath);
	
	// Shrink file path to fit on console
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hStdOut, &si))
		FSF.TruncPathStr(context->wszFilePath, si.dwSize.X - 16);

	return ExtractProgress((HANDLE)context, 0);
}

static void CALLBACK ExtractDone(ProgressContext* context, HANDLE screen, bool success)
{
	FarSInfo.RestoreScreen(screen);

	if (success)
		context->nTotalProcessedBytes += context->nCurrentFileSize;
}

static int ExtractError(int errorReason, HANDLE context)
{
	ProgressContext* pctx = (ProgressContext*) context;
	
	static const wchar_t* InfoLines[7];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_FAILED);
	InfoLines[2] = pctx->wszFilePath;
	InfoLines[3] = L"Retry";
	InfoLines[4] = GetLocMsg(MSG_BTN_SKIP);
	InfoLines[5] = GetLocMsg(MSG_BTN_SKIP_ALL);
	InfoLines[6] = GetLocMsg(MSG_BTN_ABORT);

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

static bool AskExtractOverwrite(int &overwrite, const WIN32_FIND_DATAW* existingFile, const ContentTreeNode* newFile)
{
	__int64 nOldSize = ((__int64) existingFile->nFileSizeHigh >> 32) + existingFile->nFileSizeLow;
	__int64 nNewSize = newFile->Size();
	
	SYSTEMTIME stOldUTC, stOldLocal;
	FileTimeToSystemTime(&existingFile->ftLastWriteTime, &stOldUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stOldUTC, &stOldLocal);

	SYSTEMTIME stNewUTC, stNewLocal;
	FileTimeToSystemTime(&newFile->LastModificationTime, &stNewUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stNewUTC, &stNewLocal);
	
	static wchar_t szDialogLine1[120] = {0};
	swprintf_s(szDialogLine1, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), GetLocMsg(MSG_EXTRACT_OVERWRITE), newFile->Name());
	static wchar_t szDialogLine2[120] = {0};
	swprintf_s(szDialogLine2, sizeof(szDialogLine1) / sizeof(szDialogLine1[0]), L"%21s  %s", L"Size", L"Last Modification");
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
	DlgLines[7] = GetLocMsg(MSG_BTN_SKIP);
	DlgLines[8] = GetLocMsg(MSG_BTN_SKIP_ALL);
	DlgLines[9] = GetLocMsg(MSG_BTN_CANCEL);

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

static int ExtractStorageItem(StorageObject* storage, const ContentTreeNode* item, const wchar_t* destPath, bool silent, int &doOverwrite, bool &skipOnError, HANDLE callbackContext)
{
	if (!item || !storage || item->IsDir())
		return SER_ERROR_READ;

	// Check for ESC pressed
	if (CheckEsc())	return SER_USERABORT;

	// Ask about overwrite if needed
	WIN32_FIND_DATAW fdExistingFile = {0};
	bool fAlreadyExists = FileExists(destPath, &fdExistingFile);

	if (!silent && fAlreadyExists)
	{
		if (doOverwrite == EXTR_OVERWRITE_ASK)
			if (!AskExtractOverwrite(doOverwrite, &fdExistingFile, item))
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

	// Create directory if needed
	if (!fAlreadyExists)
	{
		wstring strTargetDir = GetDirectoryName(destPath, false);
		if (strTargetDir.length() > 0)
		{
			if (!ForceDirectoryExist(strTargetDir.c_str()))
			{
				if (!silent)
					DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, strTargetDir.c_str());

				return SER_ERROR_WRITE;
			}
		}
	}

	// Remove read-only attribute from target file if present
	if (fAlreadyExists && (fdExistingFile.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
	{
		SetFileAttributes(destPath, fdExistingFile.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
	}

	ProgressContext* pctx = (ProgressContext*) callbackContext;
	HANDLE hScreen;

	int ret;
	do
	{
		// Set extract params
		ExtractOperationParams params;
		params.item = item->StorageIndex;
		params.flags = 0;
		params.destFilePath = destPath;
		params.callbacks.FileProgress = ExtractProgress;
		params.callbacks.signalContext = callbackContext;

		ExtractStart(item, pctx, hScreen);
		ret = storage->Extract(params);
		ExtractDone(pctx, hScreen, ret == SER_SUCCESS);

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
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_FAILED, pctx->wszFilePath);
		}

	} while ((ret != SER_SUCCESS) && (ret != SER_ERROR_SYSTEM) && (ret != SER_USERABORT));

	return ret;
}

int BatchExtract(StorageObject* info, ContentNodeList &items, __int64 totalExtractSize, bool silent, ExtractSelectedParams &extParams)
{
	// Items should be sorted (e.g. for access to solid archives)
	sort(items.begin(), items.end());

	if (!ForceDirectoryExist(extParams.strDestPath.c_str()))
	{
		if (!silent)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, NULL);
		return 0;
	}

	if (!IsEnoughSpaceInPath(extParams.strDestPath.c_str(), totalExtractSize))
	{
		DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_NODISKSPACE, NULL);
		return 0;
	}

	int nExtractResult = SER_SUCCESS;
	bool skipOnError = false;

	int doOverwrite = EXTR_OVERWRITE_ASK;
	switch (extParams.nOverwriteExistingFiles)
	{
		case 0:
			doOverwrite = EXTR_OVERWRITE_SKIPSILENT;
			break;
		case 1:
			doOverwrite = EXTR_OVERWRITE_SILENT;
			break;
	}

	ProgressContext pctx;
	pctx.nTotalFiles = (int) items.size();
	pctx.nTotalSize = totalExtractSize;

	// Win7 only feature
	FarSInfo.AdvControl(FarSInfo.ModuleNumber, ACTL_SETPROGRESSSTATE, (totalExtractSize > 0) ? (void*) PS_NORMAL : (void*) PS_INDETERMINATE);

	wchar_t wszSaveTitle[512], wszCurTitle[128];
	GetConsoleTitle(wszSaveTitle, ARRAY_SIZE(wszSaveTitle));

	// Extract all files one by one
	for (ContentNodeList::const_iterator cit = items.begin(); cit != items.end(); cit++)
	{
		swprintf_s(wszCurTitle, ARRAY_SIZE(wszCurTitle), L"Extracting Files (%d / %d)", pctx.nCurrentFileNumber, pctx.nTotalFiles);
		SetConsoleTitle(wszCurTitle);
		
		ContentTreeNode* nextItem = *cit;
		wstring strFullTargetPath = GetFinalExtractionPath(info, nextItem, extParams.strDestPath.c_str(), extParams.nPathProcessing);
		
		if (nextItem->IsDir())
		{
			if (!ForceDirectoryExist(strFullTargetPath.c_str()))
				return 0;
		}
		else
		{
			nExtractResult = ExtractStorageItem(info, nextItem, strFullTargetPath.c_str(), silent, doOverwrite, skipOnError, &pctx);
		}

		if (nExtractResult != SER_SUCCESS) break;
	}

	SetConsoleTitle(wszSaveTitle);
	FarSInfo.AdvControl(FarSInfo.ModuleNumber, ACTL_SETPROGRESSSTATE, (void*) PS_NOPROGRESS);
	FarSInfo.AdvControl(FarSInfo.ModuleNumber, ACTL_PROGRESSNOTIFY, 0);

	if (nExtractResult == SER_USERABORT)
		return -1;
	else if (nExtractResult == SER_SUCCESS)
		return 1;

	return 0;
}

bool ConfirmExtract(int NumFiles, int NumDirectories, ExtractSelectedParams &params)
{
	static wchar_t szDialogLine1[120] = {0};
	swprintf_s(szDialogLine1, ARRAY_SIZE(szDialogLine1), GetLocMsg(MSG_EXTRACT_CONFIRM), NumFiles, NumDirectories);
		
	FarDialogItem DialogItems []={
		/*0*/{DI_DOUBLEBOX, 3, 1, 56, 9, 0, 0, 0,0, GetLocMsg(MSG_EXTRACT_TITLE)},
		/*1*/{DI_TEXT,	    5, 2,  0, 2, 0, 0, 0, 0, szDialogLine1, 0},
		/*2*/{DI_EDIT,	    5, 3, 53, 3, 1, 0, DIF_EDITEXPAND|DIF_EDITPATH,0, params.strDestPath.c_str(), 0},
		/*3*/{DI_TEXT,	    3, 4,  0, 4, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, L""},
		/*4*/{DI_CHECKBOX,  5, 5,  0, 5, 0, params.nOverwriteExistingFiles, DIF_3STATE, 0, GetLocMsg(MSG_EXTRACT_DEFOVERWRITE)},
		/*5*/{DI_CHECKBOX,  5, 6,  0, 6, 0, params.nPathProcessing, DIF_3STATE, 0, GetLocMsg(MSG_EXTRACT_KEEPPATHS)},
		/*6*/{DI_TEXT,	    3, 7,  0, 7, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, L"", 0},
		/*7*/{DI_BUTTON,	0, 8,  0, 8, 0, 0, DIF_CENTERGROUP, 1, GetLocMsg(MSG_BTN_EXTRACT), 0},
		/*8*/{DI_BUTTON,    0, 8,  0, 8, 0, 0, DIF_CENTERGROUP, 0, GetLocMsg(MSG_BTN_CANCEL), 0},
	};

	HANDLE hDlg = FarSInfo.DialogInit(FarSInfo.ModuleNumber, -1, -1, 60, 11, L"ObserverExtract",
		DialogItems, sizeof(DialogItems) / sizeof(DialogItems[0]), 0, 0, FarSInfo.DefDlgProc, 0);

	bool retVal = false;
	if (hDlg != INVALID_HANDLE_VALUE)
	{
		int ExitCode = FarSInfo.DialogRun(hDlg);
		if (ExitCode == 7) // OK was pressed
		{
			params.nOverwriteExistingFiles = DlgHlp_GetCheckBoxState(hDlg, 4);
			params.nPathProcessing = DlgHlp_GetCheckBoxState(hDlg, 5);
			DlgHlp_GetEditBoxText(hDlg, 2, params.strDestPath);

			params.strDestPath = ResolveFullPath(params.strDestPath.c_str());

			retVal = true;
		}
		FarSInfo.DialogFree(hDlg);
	}

	return retVal;
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
		CutFileNameFromPath(wszPluginLocation, true);
	}
	else
	{
		wmemset(wszPluginLocation, 0, MAX_PATH);
	}
	swprintf_s(wszConfigFileLocation, ARRAY_SIZE(wszConfigFileLocation), L"%s%s", wszPluginLocation, CONFIG_FILE);

	LoadSettings();
	g_pController.Init(wszPluginLocation, wszConfigFileLocation);
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
	/*00*/ {DI_DOUBLEBOX, 3,1, 40, 8, 0, 0, 0,0, GetLocMsg(MSG_CONFIG_TITLE), 0},
	/*01*/ {DI_CHECKBOX,  5,2,  0, 2, 1, optEnabled, 0,0, GetLocMsg(MSG_CONFIG_ENABLE), 0},
	/*02*/ {DI_CHECKBOX,  5,3,  0, 3, 0, optUsePrefix, 0,0, GetLocMsg(MSG_CONFIG_PREFIX), 0},
	/*03*/ {DI_FIXEDIT,   8,4, 24, 4, 0, 0, 0,0, optPrefix, 0},
	/*04*/ {DI_CHECKBOX,  5,5,  0, 5, 0, optUseExtensionFilters, 0,0, GetLocMsg(MSG_CONFIG_USEEXTFILTERS), 0},
	/*05*/ {DI_TEXT,	  3,6,  0, 6, 0, 0, DIF_BOXCOLOR|DIF_SEPARATOR, 0, L"", 0},
	/*06*/ {DI_BUTTON,	  0,7,  0, 7, 0, 0, DIF_CENTERGROUP, 1, L"OK", 0},
	/*07*/ {DI_BUTTON,    0,7,  0, 7, 0, 0, DIF_CENTERGROUP, 0, GetLocMsg(MSG_BTN_CANCEL), 0},
	};

	HANDLE hDlg = FarSInfo.DialogInit(FarSInfo.ModuleNumber, -1, -1, 44, 10, L"ObserverConfig",
		DialogItems, sizeof(DialogItems) / sizeof(DialogItems[0]), 0, 0, FarSInfo.DefDlgProc, 0);

	if (hDlg != INVALID_HANDLE_VALUE)
	{
		int ExitCode = FarSInfo.DialogRun(hDlg);
		if (ExitCode == 6) // OK was pressed
		{
			optEnabled = DlgHlp_GetCheckBoxState(hDlg, 1);
			optUsePrefix = DlgHlp_GetCheckBoxState(hDlg, 2);
			DlgHlp_GetEditBoxText(hDlg, 3, optPrefix, ARRAY_SIZE(optPrefix));
			
			SaveSettings();
		}
		FarSInfo.DialogFree(hDlg);
		
		if (ExitCode == 6) return TRUE;
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
	
	wstring strFullSourcePath;
	wstring strSubPath;
	int nOpenModuleIndex = -1;
	
	if ((OpenFrom == OPEN_COMMANDLINE) && optUsePrefix)
	{
		wchar_t* szLocalNameBuffer = _wcsdup((wchar_t *) Item);
		FSF.Unquote(szLocalNameBuffer);

		// Find starting subdirectory if specified
		wchar_t* wszColonPos = wcsrchr(szLocalNameBuffer, ':');
		if (wszColonPos != NULL && (wszColonPos - szLocalNameBuffer) > 2)
		{
			*wszColonPos = 0;
			strSubPath = wszColonPos + 1;
		}

		strFullSourcePath = ResolveFullPath(szLocalNameBuffer);

		free(szLocalNameBuffer);
	}
	else if (OpenFrom == OPEN_PLUGINSMENU)
	{
		if (GetSelectedPanelFilePath(strFullSourcePath))
		{
			FarMenuItem MenuItems[2] = {0};
			MenuItems[0].Text = GetLocMsg(MSG_OPEN_FILE);
			MenuItems[1].Text = GetLocMsg(MSG_OPEN_FILE_AS);

			int nMItem = FarSInfo.Menu(FarSInfo.ModuleNumber, -1, -1, 0, 0, GetLocMsg(MSG_PLUGIN_NAME), NULL, NULL, NULL, NULL, MenuItems, ARRAY_SIZE(MenuItems));
			
			if (nMItem == -1)
				return INVALID_HANDLE_VALUE;
			else if (nMItem == 1) // Show modules selection dialog
			{
				int nSelectedModItem = SelectModuleToOpenFileAs();
				if (nSelectedModItem >= 0)
					nOpenModuleIndex = nSelectedModItem;
				else
					return INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			//Display massage about incompatible object
		}
	}

	HANDLE hOpenResult = (strFullSourcePath.size() > 0) ? OpenStorage(strFullSourcePath.c_str(), false, nOpenModuleIndex) : INVALID_HANDLE_VALUE;

	if ( (hOpenResult != INVALID_HANDLE_VALUE) && (strSubPath.size() > 0) )
		SetDirectoryW(hOpenResult, strSubPath.c_str(), 0);

	return hOpenResult;
}

HANDLE WINAPI OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	if (!Name || !optEnabled)
		return INVALID_HANDLE_VALUE;

	HANDLE hOpenResult = OpenStorage(Name, optUseExtensionFilters != 0, -1);
	return hOpenResult;
}

int WINAPI GetFindDataW(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	StorageObject* info = (StorageObject *) hPlugin;
	if (!info || !info->CurrentDir()) return FALSE;

	size_t nTotalItems = info->CurrentDir()->GetChildCount();
	*pItemsNumber = (int) nTotalItems;

	// Zero items - exit now
	if (nTotalItems == 0) return TRUE;

	*pPanelItem = (PluginPanelItem *) malloc(nTotalItems * sizeof(PluginPanelItem));
	PluginPanelItem *panelItem = *pPanelItem;

	// Display all directories
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->subdirs.begin(); cit != info->CurrentDir()->subdirs.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		ContentTreeNode* node = (cit->second);

		panelItem->FindData.lpwszFileName = node->Name();
		panelItem->FindData.dwFileAttributes = node->Attributes;
		panelItem->FindData.ftCreationTime = node->CreationTime;
		panelItem->FindData.ftLastWriteTime = node->LastModificationTime;
		panelItem->FindData.nFileSize = node->Size();

		panelItem++;
	}

	// Display all files
	for (SubNodesMap::const_iterator cit = info->CurrentDir()->files.begin(); cit != info->CurrentDir()->files.end(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		ContentTreeNode* node = (cit->second);

		panelItem->FindData.lpwszFileName = node->Name();
		panelItem->FindData.dwFileAttributes = node->Attributes;
		panelItem->FindData.ftCreationTime = node->CreationTime;
		panelItem->FindData.ftLastWriteTime = node->LastModificationTime;
		panelItem->FindData.nFileSize = node->Size();

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
	if (hPlugin == NULL || hPlugin == INVALID_HANDLE_VALUE)
		return FALSE;

	StorageObject* info = (StorageObject *) hPlugin;
	if (!info) return FALSE;

	if (!Dir || !*Dir) return TRUE;

	if (optExtendedCurDir)
	{
		// If we are in root directory and wanna go upwards then we should close plugin panel
		if (wcscmp(Dir, L"..") == 0 && info->CurrentDir()->parent == NULL)
		{
			wchar_t* wszStoragePath = _wcsdup(info->StoragePath());
			CutFileNameFromPath(wszStoragePath, false);

			FarSInfo.Control(hPlugin, FCTL_CLOSEPLUGIN, 0, (LONG_PTR) wszStoragePath);
			FarSInfo.Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, (LONG_PTR) wszStoragePath);

			// Find position of our container on panel and position cursor there
			PanelInfo pi = {0};
			if (FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi))
			{
				for (int i = 0; i < pi.ItemsNumber; i++)
				{
					PluginPanelItem *PPI = (PluginPanelItem*)malloc(FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, NULL));
					if (!PPI) break;

					FarSInfo.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, (LONG_PTR)PPI);
					bool fIsArchItem = wcscmp(ExtractFileName(info->StoragePath()), PPI->FindData.lpwszFileName) == 0;
					free(PPI);

					if (fIsArchItem)
					{
						PanelRedrawInfo pri = {0};
						pri.CurrentItem = i;
						FarSInfo.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR) &pri);
						break;
					}
				} // for
			}

			free(wszStoragePath);
			return TRUE;
		}
		else
		{
			const wchar_t* wszInsideDirPart = wcsrchr(Dir, ':');
			if (wszInsideDirPart && (wszInsideDirPart - Dir) > 2)
				return info->ChangeCurrentDir(wszInsideDirPart+1);
		}
	}

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
	
	static wchar_t wszCurrentDir[PATH_BUFFER_SIZE];
	static wchar_t wszTitle[512];
	static KeyBarTitles KeyBar;
	
	static wchar_t wszStorageSizeInfo[32];
	static wchar_t wszNumFileInfo[16];
	static wchar_t wszNumDirsInfo[16];
	static wchar_t wszStorageCreatedInfo[32];

	memset(wszCurrentDir, 0, sizeof(wszCurrentDir));
	memset(wszTitle, 0, sizeof(wszTitle));

	if (optExtendedCurDir && optUsePrefix && optPrefix[0])
		swprintf_s(wszCurrentDir, ARRAY_SIZE(wszCurrentDir), L"%s:%s:\\", optPrefix, info->StoragePath());
	else
		wszCurrentDir[0] = '\\';

	size_t nDirPrefixSize = wcslen(wszCurrentDir);
	info->CurrentDir()->GetPath(wszCurrentDir + nDirPrefixSize, PATH_BUFFER_SIZE - nDirPrefixSize);

	// Set title
	swprintf_s(wszTitle, ARRAY_SIZE(wszTitle), L"%s%s:%s", optPanelHeaderPrefix, info->GetModuleName(), wszCurrentDir);

	// FAR does not exit plug-in if root directory is "\"
	if (wcslen(wszCurrentDir) == 1) wszCurrentDir[0] = 0;

	_i64tow_s(info->TotalSize(), wszStorageSizeInfo, ARRAY_SIZE(wszStorageSizeInfo), 10);
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

	pInfoLinesData[0].Text = ExtractFileName(info->StoragePath());
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
	Info->HostFile = info->StoragePath();
	Info->InfoLinesNumber = ARRAY_SIZE(pInfoLinesData);
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

	ContentNodeList vcExtractItems;
	__int64 nTotalExtractSize = 0;
	int nExtNumFiles = 0, nExtNumDirs = 0;

	// Collect all indices of items to extract
	for (int i = 0; i < ItemsNumber; i++)
	{
		PluginPanelItem pItem = PanelItem[i];
		if (wcscmp(pItem.FindData.lpwszFileName, L"..") == 0) continue;

		ContentTreeNode* child = info->CurrentDir()->GetChildByName(pItem.FindData.lpwszFileName);
		if (child)
		{
			CollectFileList(child, vcExtractItems, nTotalExtractSize, true);

			if (pItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				nExtNumDirs++;
			else
				nExtNumFiles++;
		}
	} //for

	// Check if we have something to extract
	if (vcExtractItems.size() == 0)
		return 0;

	ExtractSelectedParams extParams(*DestPath);

	// Confirm extraction
	if ((OpMode & OPM_SILENT) == 0)
	{
		if (!ConfirmExtract(nExtNumFiles, nExtNumDirs, extParams))
			return -1;
	}

	return BatchExtract(info, vcExtractItems, nTotalExtractSize, (OpMode & OPM_SILENT) > 0, extParams);
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

		ContentNodeList vcExtractItems;
		__int64 nTotalExtractSize = 0;

		// Collect all indices of items to extract
		for (int i = 0; i < pi.SelectedItemsNumber; i++)
		{
			PluginPanelItem* pItem = (PluginPanelItem*) malloc(FarSInfo.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, NULL));
			
			FarSInfo.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR) pItem);
			if (wcscmp(pItem->FindData.lpwszFileName, L"..") != 0)
			{
				ContentTreeNode* child = info->CurrentDir()->GetChildByName(pItem->FindData.lpwszFileName);
				if (child) CollectFileList(child, vcExtractItems, nTotalExtractSize, true);
			}
			
			free(pItem);
		} //for

		// Check if we have something to extract
		if (vcExtractItems.size() == 0) return TRUE;

		wchar_t *wszTargetDir = _wcsdup(info->StoragePath());
		CutFileNameFromPath(wszTargetDir, true);
		
		ExtractSelectedParams extParams(wszTargetDir);

		BatchExtract(info, vcExtractItems, nTotalExtractSize, true, extParams);

		free(wszTargetDir);
		
		return TRUE;
	}

	return FALSE;
}
