// Observer.cpp : Defines the exported functions for the DLL application.
// This module contains functions for Far 3.0+

#include "StdAfx.h"
#include <far3/plugin.hpp>
#include <far3/DlgBuilder.hpp>
#include <far3/PluginSettings.hpp>

#include "ModulesController.h"
#include "PlugLang.h"
#include "FarStorage.h"
#include "CommonFunc.h"
#include "InterfaceCommon.h"

#include "Config.h"

#include <InitGuid.h>
#include "Guids.h"

#include "plug_version.h"

extern HMODULE g_hDllHandle;
static PluginStartupInfo FarSInfo;
static FarStandardFunctions FSF;

static wchar_t wszPluginLocation[MAX_PATH];
static ModulesController g_pController;

// Settings
#define MAX_PREFIX_SIZE 32
static int optOpenOnEnter = TRUE;
static int optOpenOnCtrlPgDn = TRUE;
static int optUsePrefix = TRUE;
static int optUseExtensionFilters = TRUE;
static int optVerboseModuleLoad = FALSE;
static wchar_t optPrefix[MAX_PREFIX_SIZE] = L"observe";

// Extended settings
static wchar_t optPanelHeaderPrefix[MAX_PREFIX_SIZE] = L"";

//-----------------------------------  Local functions ----------------------------------------

static const wchar_t* GetLocMsg(int MsgID)
{
	return FarSInfo.GetMsg(&OBSERVER_GUID, MsgID);
}

static void LoadSettings(Config* cfg)
{
	// Load static settings from .ini file.
	ConfigSection* generalCfg = cfg->GetSection(L"General");
	if (generalCfg != NULL)
	{
		generalCfg->GetValue(L"PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE);
		generalCfg->GetValue(L"UseExtensionFilters", optUseExtensionFilters);
		generalCfg->GetValue(L"VerboseModuleLoad", optVerboseModuleLoad);
	}

	// Load dynamic settings from registry (they will overwrite static ones)
	PluginSettings ps(OBSERVER_GUID, FarSInfo.SettingsControl);

	optOpenOnEnter = ps.Get(0, L"OpenOnEnter", optOpenOnEnter);
	optOpenOnCtrlPgDn = ps.Get(0, L"OpenOnCtrlPgdn", optOpenOnCtrlPgDn);
	optUsePrefix = ps.Get(0, L"UsePrefix", optUsePrefix);
	ps.Get(0, L"Prefix", optPrefix, MAX_PREFIX_SIZE, optPrefix);

	ps.Get(0, L"PanelHeaderPrefix", optPanelHeaderPrefix, MAX_PREFIX_SIZE, optPanelHeaderPrefix);
	optUseExtensionFilters = ps.Get(0, L"UseExtensionFilters", optUseExtensionFilters);
}

static void SaveSettings()
{
	PluginSettings ps(OBSERVER_GUID, FarSInfo.SettingsControl);
	
	ps.Set(0, L"OpenOnEnter", optOpenOnEnter);
	ps.Set(0, L"OpenOnCtrlPgdn", optOpenOnCtrlPgDn);
	ps.Set(0, L"UsePrefix", optUsePrefix);
	ps.Set(0, L"Prefix", optPrefix);
	ps.Set(0, L"UseExtensionFilters", optUseExtensionFilters);
}

static wstring ResolveFullPath(const wchar_t* input)
{
	wstring strFull;
	size_t nLen = FSF.ConvertPath(CPM_FULL, input, NULL, 0);
	if (nLen > 0)
	{
		wchar_t* pszFull = (wchar_t*) calloc(nLen, sizeof(*input));
		FSF.ConvertPath(CPM_FULL, input, pszFull, nLen);
		strFull = pszFull;
		free(pszFull);
	}
	else
	{
		strFull = input;
	}
	return strFull;
}

static void DisplayMessage(bool isError, bool isInteractive, const wchar_t* headerMsg, const wchar_t* textMsg, const wchar_t* errorItem)
{
	static const wchar_t* MsgLines[3];
	MsgLines[0] = headerMsg;
	MsgLines[1] = textMsg;
	MsgLines[2] = errorItem;

	int linesNum = (errorItem) ? 3 : 2;
	int flags = 0;
	if (isError) flags |= FMSG_WARNING;
	if (isInteractive) flags |= FMSG_MB_OK;

	FarSInfo.Message(&OBSERVER_GUID, &GUID_OBS_MESSAGE_BOX, flags, NULL, MsgLines, linesNum, 0);
}

static void DisplayMessage(bool isError, bool isInteractive, int headerMsgID, int textMsgID, const wchar_t* errorItem)
{
	DisplayMessage(isError, isInteractive, GetLocMsg(headerMsgID), GetLocMsg(textMsgID), errorItem);
}

static int SelectModuleToOpenFileAs()
{
	size_t nNumModules = g_pController.NumModules();
	
	FarMenuItem* MenuItems = new FarMenuItem[nNumModules];
	vector<wstring> MenuStrings(nNumModules);

	memset(MenuItems, 0, nNumModules * sizeof(FarMenuItem));
	for (size_t i = 0; i < nNumModules; i++)
	{
		const ExternalModule* modInfo = g_pController.GetModule((int) i);
		if (modInfo->ShortCut)
			MenuStrings[i] = FormatString(L"&%c %s", modInfo->ShortCut, modInfo->Name());
		else
			MenuStrings[i] = modInfo->Name();
		MenuItems[i].Text = MenuStrings[i].c_str();
	}

	intptr_t nMSel = FarSInfo.Menu(&OBSERVER_GUID, &GUID_OBS_MENU, -1, -1, 0, 0, GetLocMsg(MSG_OPEN_SELECT_MODULE), NULL, NULL, NULL, NULL, MenuItems, (int) nNumModules);

	delete [] MenuItems;
	return (int) nMSel;
}

static bool StoragePasswordQuery(char* buffer, size_t bufferSize)
{
	wchar_t passBuf[100] = {0};

	intptr_t nRet = FarSInfo.InputBox(&OBSERVER_GUID, &GUID_OBS_INPUTBOX, GetLocMsg(MSG_PLUGIN_NAME), GetLocMsg(MSG_OPEN_PASS_REQUIRED), NULL, NULL, passBuf, ARRAY_SIZE(passBuf)-1, NULL, FIB_PASSWORD | FIB_NOUSELASTHISTORY);
	if (nRet == TRUE)
	{
		memset(buffer, 0, bufferSize);
		WideCharToMultiByte(CP_ACP, 0, passBuf, -1, buffer, (int) bufferSize - 1, NULL, NULL);
		return true;
	}
	
	return false;
}

static void ReportFailedModules(std::vector<FailedModuleInfo> &failedModules)
{
	if (!optVerboseModuleLoad || (failedModules.size() == 0)) return;

	size_t boxListSize = failedModules.size() * 2;
	const wchar_t* *boxList = new const wchar_t*[boxListSize];
	
	size_t listIndex = 0;
	for (size_t i = 0; i < failedModules.size(); i++)
	{
		boxList[listIndex] = failedModules[i].ModuleName.c_str();
		boxList[listIndex+1] = failedModules[i].ErrorMessage.c_str();
		listIndex += 2;
	}

	PluginDialogBuilder Builder(FarSInfo, OBSERVER_GUID, GUID_OBS_LOAD_ERROR, L"Loading Error", nullptr, nullptr, nullptr, FDLG_WARNING);

	Builder.AddText(L"Some modules failed to load");
	Builder.AddListBox(nullptr, 50, 10, boxList, boxListSize, DIF_LISTNOBOX | DIF_LISTNOCLOSE);
	Builder.AddOKCancel(MSG_BTN_OK, -1, -1, true);

	Builder.ShowDialog();

	delete [] boxList;
}

//-----------------------------------  Content functions ----------------------------------------

static int AnalizeStorage(const wchar_t* Name, bool applyExtFilters, void* startBuffer, size_t startBufferSize)
{
	StorageObject *storage = new StorageObject(&g_pController, StoragePasswordQuery);
	
	bool openOk = storage->Open(Name, startBuffer, startBufferSize, applyExtFilters, -1);
	int retVal = openOk ? storage->GetModuleIndex() : -1;

	delete storage;
	return retVal;
}

static HANDLE OpenStorage(const wchar_t* Name, bool applyExtFilters, int moduleIndex)
{
	StorageObject *storage = new StorageObject(&g_pController, StoragePasswordQuery);
	if (!storage->Open(Name, applyExtFilters, moduleIndex))
	{
		delete storage;
		return INVALID_HANDLE_VALUE;
	}

	wchar_t wszDialogText[100] = {0};
	wchar_t wszConTitleText[100] = {0};

	swprintf_s(wszDialogText, ARRAY_SIZE(wszDialogText), GetLocMsg(MSG_OPEN_LIST), storage->GetModuleName());
	swprintf_s(wszConTitleText, ARRAY_SIZE(wszConTitleText), L"%s: %s", GetLocMsg(MSG_PLUGIN_NAME), wszDialogText);

	HANDLE hScreen = FarSInfo.SaveScreen(0, 0, -1, -1);
	DisplayMessage(false, false, GetLocMsg(MSG_PLUGIN_NAME), wszDialogText, NULL);
	SaveConsoleTitle ct(wszConTitleText);

	FarSInfo.AdvControl(&OBSERVER_GUID, ACTL_SETPROGRESSSTATE, TBPS_INDETERMINATE, NULL);

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

	FarSInfo.AdvControl(&OBSERVER_GUID, ACTL_SETPROGRESSSTATE, TBPS_NOPROGRESS, NULL);
	FarSInfo.RestoreScreen(hScreen);

	return hResult;
}

static void CloseStorage(HANDLE hStorage)
{
	StorageObject *sobj = (StorageObject*) hStorage;
	sobj->Close();
	delete sobj;
}

static bool GetCurrentPanelItemName(HANDLE hPanel, wstring& nameStr, bool canBeDir)
{
	bool fResult = false;
	
	size_t itemBufSize = FarSInfo.PanelControl(hPanel, FCTL_GETCURRENTPANELITEM, 0, NULL);
	PluginPanelItem *PPI = (PluginPanelItem*) malloc(itemBufSize);
	if (PPI)
	{
		FarGetPluginPanelItem FGPPI = {sizeof(FarGetPluginPanelItem), itemBufSize, PPI};
		FarSInfo.PanelControl(hPanel, FCTL_GETCURRENTPANELITEM, 0, &FGPPI);
		if (canBeDir || ((PPI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
		{
			nameStr = PPI->FileName;
			fResult = true;
		}
		free(PPI);
	}

	return fResult;
}

static bool GetSelectedPanelFilePath(wstring& nameStr)
{
	nameStr.clear();
	
	PanelInfo pi = {sizeof(PanelInfo)};
	if (FarSInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi))
		if ((pi.SelectedItemsNumber == 1) && (pi.PanelType == PTYPE_FILEPANEL))
		{
			intptr_t dirBufSize = FarSInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, 0, NULL);
			FarPanelDirectory *panelDir = (FarPanelDirectory*) malloc(dirBufSize);
			if (panelDir)
			{
				panelDir->StructSize = sizeof(FarPanelDirectory);
				FarSInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, dirBufSize, panelDir);

				wstring strNameBuffer = panelDir->Name;
				IncludeTrailingPathDelim(strNameBuffer);

				wstring itemName;
				if (GetCurrentPanelItemName(PANEL_ACTIVE, itemName, false))
				{
					strNameBuffer.append(itemName);
					nameStr = strNameBuffer;
				}
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

	if (pc->bDisplayOnScreen && (nFileProgress != pc->nCurrentProgress))
	{
		static wchar_t szFileProgressLine[100] = {0};
		swprintf_s(szFileProgressLine, ARRAY_SIZE(szFileProgressLine), L"File: %d/%d. Progress: %2d%% / %2d%%", pc->nCurrentFileNumber, pc->nTotalFiles, nFileProgress, nTotalProgress);

		static const wchar_t* InfoLines[4];
		InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
		InfoLines[1] = GetLocMsg(MSG_EXTRACT_EXTRACTING);
		InfoLines[2] = szFileProgressLine;
		InfoLines[3] = pc->wszFilePath;

		FarSInfo.Message(&OBSERVER_GUID, &GUID_OBS_ERROR_MESSAGE, 0, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 0);

		// Win7 only feature
		if (pc->nTotalSize > 0)
		{
			ProgressValue pv;
			pv.StructSize = sizeof(ProgressValue);
			pv.Completed = pc->nTotalProcessedBytes + pc->nProcessedFileBytes;
			pv.Total = pc->nTotalSize;
			FarSInfo.AdvControl(&OBSERVER_GUID, ACTL_SETPROGRESSVALUE, 0, &pv);
		}
	}

	pc->nCurrentProgress = nFileProgress;
	return TRUE;
}

static int CALLBACK ExtractStart(const ContentTreeNode* item, ProgressContext* context, HANDLE &screen)
{
	screen = FarSInfo.SaveScreen(0, 0, -1, -1);

	context->nCurrentFileNumber++;
	context->nCurrentFileSize = item->GetSize();
	context->nProcessedFileBytes = 0;
	context->nCurrentProgress = -1;
	
	wchar_t wszSubPath[PATH_BUFFER_SIZE] = {0};
	item->GetPath(wszSubPath, PATH_BUFFER_SIZE);
		
	// Shrink file path to fit on console
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hStdOut, &si))
	{
		FSF.TruncPathStr(wszSubPath, si.dwSize.X - 16);
		wcscpy_s(context->wszFilePath, ARRAY_SIZE(context->wszFilePath), wszSubPath);
	}
	else
	{
		// Save file name for dialogs
		size_t nPathLen = wcslen(wszSubPath);
		wchar_t* wszSubPathPtr = wszSubPath;
		if (nPathLen > MAX_PATH) wszSubPathPtr += (nPathLen % MAX_PATH);

		wcscpy_s(context->wszFilePath, ARRAY_SIZE(context->wszFilePath), wszSubPathPtr);
	}

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

	intptr_t nMsg = FarSInfo.Message(&OBSERVER_GUID, &GUID_OBS_ERROR_MESSAGE, FMSG_WARNING, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 4);

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
enum FileOverwriteOptions
{
	OverwriteAsk = 1,
	OverwriteSilent = 2,
	OverwriteSkip = 3,
	OverwriteSkipSilent = 4,
	OverwriteRename = 5
};

static bool AskExtractOverwrite(FileOverwriteOptions &overwrite, wstring &destPath, const WIN32_FIND_DATAW* existingFile, const ContentTreeNode* newFile)
{
	__int64 nOldSize = ((__int64) existingFile->nFileSizeHigh >> 32) + existingFile->nFileSizeLow;
	__int64 nNewSize = newFile->GetSize();
	
	SYSTEMTIME stOldUTC, stOldLocal;
	FileTimeToSystemTime(&existingFile->ftLastWriteTime, &stOldUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stOldUTC, &stOldLocal);

	SYSTEMTIME stNewUTC, stNewLocal;
	FileTimeToSystemTime(&newFile->LastModificationTime, &stNewUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stNewUTC, &stNewLocal);
	
	static wchar_t szOldFileInfo[120] = {0};
	swprintf_s(szOldFileInfo, ARRAY_SIZE(szOldFileInfo), L"%-12s %14u  %02u-%02u-%4u %02u:%02u",
		GetLocMsg(MSG_DLG_OLDFILE_INFO), (UINT) nOldSize, stOldLocal.wDay, stOldLocal.wMonth, stOldLocal.wYear, stOldLocal.wHour, stOldLocal.wMinute);
	static wchar_t szNewFileInfo[120] = {0};
	swprintf_s(szNewFileInfo, ARRAY_SIZE(szNewFileInfo), L"%-12s %14u  %02u-%02u-%4u %02u:%02u",
		GetLocMsg(MSG_DLG_NEWFILE_INFO), (UINT) nNewSize, stNewLocal.wDay, stNewLocal.wMonth, stNewLocal.wYear, stNewLocal.wHour, stNewLocal.wMinute);

	int aButtons[] = {MSG_OVERWRITE, MSG_BTN_SKIP, MSG_BTN_RENAME, MSG_BTN_CANCEL};
	int nNumButtons = ARRAY_SIZE(aButtons);
	int fRememberChoice = 0;

	PluginDialogBuilder Builder(FarSInfo, OBSERVER_GUID, GUID_OBS_OTHER_DIALOG, GetLocMsg(MSG_TITLE_WARNING), nullptr, nullptr, nullptr, FDLG_WARNING);

	Builder.AddText(MSG_EXTRACT_OVERWRITE)->Flags = DIF_CENTERTEXT;
	Builder.AddText(destPath.c_str());
	
	Builder.AddSeparator();
	Builder.AddText(szNewFileInfo);
	Builder.AddText(szOldFileInfo);
	
	Builder.AddSeparator();
	Builder.AddCheckbox(MSG_DLG_REMEMBER, &fRememberChoice)->Flags = DIF_FOCUS;

	Builder.AddSeparator();
	Builder.AddButtons(nNumButtons, aButtons, 0, nNumButtons - 1);

	intptr_t dlgResult = Builder.ShowDialogEx();

	switch(dlgResult)
	{
		case 0:
			overwrite = fRememberChoice ? OverwriteSilent : OverwriteAsk;
			break;
		case 1:
			overwrite = fRememberChoice ? OverwriteSkipSilent : OverwriteSkip;
			break;
		case 2:
			overwrite = OverwriteRename;
			break;
		default:
			return false;
	}

	return true;
}

static void AskRename(wstring &filePath)
{
	wchar_t tmpBuf[MAX_PATH] = {0};
	wcscpy_s(tmpBuf, MAX_PATH, ExtractFileName(filePath.c_str()));
	
	PluginDialogBuilder Builder(FarSInfo, OBSERVER_GUID, GUID_OBS_OTHER_DIALOG, GetLocMsg(MSG_TITLE_RENAME), NULL);

	Builder.AddText(MSG_DLG_NEWFILENAME);
	Builder.AddEditField(tmpBuf, MAX_PATH, 50)->Flags = DIF_FOCUS;
	Builder.AddOKCancel(MSG_BTN_RENAME, MSG_BTN_CANCEL, -1, false);

	if (Builder.ShowDialog())
	{
		filePath = GetDirectoryName(filePath, true) + tmpBuf;
	}
}

static int ExtractStorageItem(StorageObject* storage, const ContentTreeNode* item, wstring &destPath, bool showMessages, FileOverwriteOptions &doOverwrite, bool &skipOnError, ProgressContext *pctx)
{
	if (!item || !storage || item->IsDir())
		return SER_ERROR_READ;

	// Check for ESC pressed
	if (CheckEsc())	return SER_USERABORT;

	// Ask about overwrite if needed
	WIN32_FIND_DATAW fdExistingFile = {0};
	bool fAlreadyExists;

	while ((fAlreadyExists = FileExists(destPath, &fdExistingFile)))
	{
		if (doOverwrite == OverwriteAsk)
		{
			if (!showMessages) break;

			if (!AskExtractOverwrite(doOverwrite, destPath, &fdExistingFile, item))
				return SER_USERABORT;
		}
		
		// Check either ask result or present value
		if (doOverwrite == OverwriteSkip)
		{
			doOverwrite = OverwriteAsk;
			return SER_SUCCESS;
		}
		else if (doOverwrite == OverwriteSkipSilent)
		{
			return SER_SUCCESS;
		}
		else if (doOverwrite == OverwriteRename)
		{
			// Ask next time
			doOverwrite = OverwriteAsk;
			AskRename(destPath);
		}
		else
		{
			// This is one of the overwrite options
			break;
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
				if (showMessages)
					DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, strTargetDir.c_str());

				return SER_ERROR_WRITE;
			}
		}
	}

	// Remove read-only attribute from target file if present
	if (fAlreadyExists && (fdExistingFile.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
	{
		SetFileAttributes(destPath.c_str(), fdExistingFile.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
	}

	HANDLE hScreen;
	string strFilePassword;

	int ret;
	do
	{
		// Set extract params
		ExtractOperationParams params;
		params.ItemIndex = item->StorageIndex;
		params.Flags = 0;
		params.DestPath = destPath.c_str();
		params.Password = (strFilePassword.length() > 0) ? strFilePassword.c_str() : nullptr;
		params.Callbacks.FileProgress = ExtractProgress;
		params.Callbacks.signalContext = pctx;

		ExtractStart(item, pctx, hScreen);
		ret = storage->Extract(params);
		ExtractDone(pctx, hScreen, ret == SER_SUCCESS);

		if ((ret == SER_ERROR_WRITE) || (ret == SER_ERROR_READ))
		{
			int errorResp = EEN_ABORT;
			if (skipOnError)
				errorResp = EEN_SKIP;
			else if (showMessages)
				errorResp = ExtractError(ret, pctx);

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
			if (showMessages)
				DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_FAILED, pctx->wszFilePath);
		}
		else if (ret == SER_PASSWORD_REQUIRED)
		{
			char passBuffer[100] = {0};
			if (StoragePasswordQuery(passBuffer, sizeof(passBuffer)))
				strFilePassword = passBuffer;
			else
				ret = SER_USERABORT;
		}

	} while ((ret != SER_SUCCESS) && (ret != SER_ERROR_SYSTEM) && (ret != SER_USERABORT));

	// If extraction is successful set file attributes if present
	if (ret == SER_SUCCESS)
	{
		if (item->GetAttributes() != 0)
			SetFileAttributes(destPath.c_str(), item->GetAttributes());
		
		UpdateFileTime(destPath.c_str(), &item->CreationTime, &item->LastModificationTime);
	}

	return ret;
}

static bool ItemSortPred(ContentTreeNode* item1, ContentTreeNode* item2)
{
	return (item1->StorageIndex < item2->StorageIndex);
}

int BatchExtract(StorageObject* info, ContentNodeList &items, __int64 totalExtractSize, ExtractSelectedParams &extParams)
{
	// Items should be sorted (e.g. for access to solid archives)
	sort(items.begin(), items.end(), ItemSortPred);

	if (!ForceDirectoryExist(extParams.strDestPath.c_str()))
	{
		if (!extParams.bSilent)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_DIR_CREATE_ERROR, NULL);
		return 0;
	}

	if (!IsEnoughSpaceInPath(extParams.strDestPath.c_str(), totalExtractSize))
	{
		if (!extParams.bSilent)
			DisplayMessage(true, true, MSG_EXTRACT_ERROR, MSG_EXTRACT_NODISKSPACE, NULL);
		return 0;
	}

	int nExtractResult = SER_SUCCESS;
	bool skipOnError = false;

	FileOverwriteOptions doOverwrite = OverwriteAsk;
	switch (extParams.nOverwriteExistingFiles)
	{
		case 0:
			doOverwrite = OverwriteSkipSilent;
			break;
		case 1:
			doOverwrite = OverwriteSilent;
			break;
	}

	ProgressContext pctx;
	pctx.nTotalFiles = (int) items.size();
	pctx.nTotalSize = totalExtractSize;
	pctx.bDisplayOnScreen = extParams.bShowProgress;

	wchar_t wszSaveTitle[512], wszCurTitle[128];
	
	if (extParams.bShowProgress)
	{
		FarSInfo.AdvControl(&OBSERVER_GUID, ACTL_SETPROGRESSSTATE, (totalExtractSize > 0) ? TBPS_NORMAL : TBPS_INDETERMINATE, NULL);
		GetConsoleTitle(wszSaveTitle, ARRAY_SIZE(wszSaveTitle));
	}

	// Extract all files one by one
	for (auto cit = items.begin(); cit != items.end(); cit++)
	{
		if (extParams.bShowProgress)
		{
			swprintf_s(wszCurTitle, ARRAY_SIZE(wszCurTitle), L"Extracting Files (%d / %d)", pctx.nCurrentFileNumber, pctx.nTotalFiles);
			SetConsoleTitle(wszCurTitle);
		}
		
		ContentTreeNode* nextItem = *cit;
		wstring strFullTargetPath = GetFinalExtractionPath(info, nextItem, extParams.strDestPath.c_str(), extParams.nPathProcessing);
		
		if (nextItem->IsDir())
		{
			if (!ForceDirectoryExist(strFullTargetPath.c_str()))
				return 0;
		}
		else
		{
			nExtractResult = ExtractStorageItem(info, nextItem, strFullTargetPath, !extParams.bSilent, doOverwrite, skipOnError, &pctx);
		}

		if (nExtractResult != SER_SUCCESS) break;
	}

	if (extParams.bShowProgress)
	{
		SetConsoleTitle(wszSaveTitle);
		FarSInfo.AdvControl(&OBSERVER_GUID, ACTL_SETPROGRESSSTATE, TBPS_NOPROGRESS, NULL);
		FarSInfo.AdvControl(&OBSERVER_GUID, ACTL_PROGRESSNOTIFY, 0, NULL);
	}

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

	wchar_t wszExtractPathBuf[MAX_PATH] = {0};
	wcscpy_s(wszExtractPathBuf, MAX_PATH, params.strDestPath.c_str());

	PluginDialogBuilder Builder(FarSInfo, OBSERVER_GUID, GUID_OBS_OTHER_DIALOG, GetLocMsg(MSG_EXTRACT_TITLE), L"ObserverExtract");

	Builder.AddText(szDialogLine1);
	Builder.AddEditField(wszExtractPathBuf, MAX_PATH, 56)->Flags |= DIF_FOCUS;
	Builder.AddSeparator();
	Builder.AddCheckbox(MSG_EXTRACT_DEFOVERWRITE, &params.nOverwriteExistingFiles, 0, true);
	Builder.AddCheckbox(MSG_EXTRACT_KEEPPATHS, &params.nPathProcessing, 0, true);
	
	Builder.AddOKCancel(MSG_BTN_EXTRACT, MSG_BTN_CANCEL, -1, true);

	if (Builder.ShowDialog())
	{
		params.strDestPath = ResolveFullPath(wszExtractPathBuf);

		return true;
	}

	return false;
}

//-----------------------------------  Export functions ----------------------------------------

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->StructSize=sizeof(GlobalInfo);
	Info->MinFarVersion = FARMANAGERVERSION;
	Info->Version = MAKEFARVERSION(OBSERVER_VERSION_MAJOR, OBSERVER_VERSION_MINOR, OBSERVER_VERSION_REVISION, 0, VS_RELEASE);
	Info->Guid = OBSERVER_GUID;
	Info->Title = L"Observer";
	Info->Description = L"Container Extractor";
	Info->Author = L"Ariman";
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

	wstring strConfigLocation(wszPluginLocation);
	
	Config cfg;
	cfg.ParseFile(strConfigLocation + CONFIG_FILE);
	cfg.ParseFile(strConfigLocation + CONFIG_USER_FILE);

	LoadSettings(&cfg);

	vector<FailedModuleInfo> fails;
	g_pController.Init(wszPluginLocation, &cfg, fails);
	ReportFailedModules(fails);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(PluginInfo);
	Info->Flags = 0;

	static const wchar_t *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetLocMsg(MSG_PLUGIN_NAME);
	
	static const wchar_t *ConfigMenuStrings[1];
	ConfigMenuStrings[0] = GetLocMsg(MSG_PLUGIN_CONFIG_NAME);

	Info->PluginMenu.Guids = &GUID_OBS_INFO_MENU;
	Info->PluginMenu.Strings = PluginMenuStrings;
	Info->PluginMenu.Count = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);
	Info->PluginConfig.Guids = &GUID_OBS_INFO_CONFIG;
	Info->PluginConfig.Strings = ConfigMenuStrings;
	Info->PluginConfig.Count = sizeof(ConfigMenuStrings) / sizeof(ConfigMenuStrings[0]);
	Info->CommandPrefix = optPrefix;
}

void WINAPI ExitFARW(const ExitInfo* Info)
{
	g_pController.Cleanup();
}

intptr_t WINAPI ConfigureW(const ConfigureInfo* Info)
{
	PluginDialogBuilder Builder(FarSInfo, OBSERVER_GUID, GUID_OBS_CONFIG_DIALOG, GetLocMsg(MSG_CONFIG_TITLE), L"ObserverConfig");

	Builder.AddCheckbox(MSG_CONFIG_OPEN_ON_ENTER, &optOpenOnEnter);
	Builder.AddCheckbox(MSG_CONFIG_OPEN_ON_CTRL_PGDN, &optOpenOnCtrlPgDn);
	Builder.AddCheckbox(MSG_CONFIG_PREFIX, &optUsePrefix);
	
	FarDialogItem* edtField = Builder.AddFixEditField(optPrefix, ARRAY_SIZE(optPrefix), 16);
	edtField->X1 += 3;

	Builder.AddCheckbox(MSG_CONFIG_USEEXTFILTERS, &optUseExtensionFilters);
	Builder.AddOKCancel(MSG_BTN_OK, MSG_BTN_CANCEL, -1, true);

	if (Builder.ShowDialog())
	{
		SaveSettings();
		
		return TRUE;
	}

	return FALSE;
}

void WINAPI ClosePanelW(const struct ClosePanelInfo* info)
{
	if (info->hPanel != NULL)
		CloseStorage(info->hPanel);
}

HANDLE WINAPI AnalyseW(const AnalyseInfo* AInfo)
{
	if (!AInfo || !AInfo->FileName)
		return nullptr;

	bool isCtrlPgdn = (AInfo->OpMode & OPM_PGDN) != 0;

	if ((isCtrlPgdn && !optOpenOnCtrlPgDn) || (!isCtrlPgdn && !optOpenOnEnter))
		return nullptr;

	int nAnalizeResult = AnalizeStorage(AInfo->FileName, optUseExtensionFilters != 0, AInfo->Buffer, AInfo->BufferSize);
	// Do not forget to decrease value by 1 in OpenW
	return (nAnalizeResult >= 0) ? (HANDLE)(nAnalizeResult + 1) : nullptr;
}

void WINAPI CloseAnalyseW(const CloseAnalyseInfo* info)
{
	// Nothing here
}

HANDLE WINAPI OpenW(const struct OpenInfo *OInfo)
{
	// Unload plug-in if no submodules loaded
	if (g_pController.NumModules() == 0)
		return 0;
	
	wstring strFullSourcePath;
	wstring strSubPath;
	int nOpenModuleIndex = -1;
	
	if ((OInfo->OpenFrom == OPEN_COMMANDLINE) && optUsePrefix)
	{
		OpenCommandLineInfo* cmdInfo = (OpenCommandLineInfo*) OInfo->Data;
		wchar_t* szLocalNameBuffer = _wcsdup(cmdInfo->CommandLine);
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
	else if (OInfo->OpenFrom == OPEN_PLUGINSMENU)
	{
		if (GetSelectedPanelFilePath(strFullSourcePath))
		{
			FarMenuItem MenuItems[2] = {0};
			MenuItems[0].Text = GetLocMsg(MSG_OPEN_FILE);
			MenuItems[1].Text = GetLocMsg(MSG_OPEN_FILE_AS);

			intptr_t nMItem = FarSInfo.Menu(&OBSERVER_GUID, &GUID_OBS_MENU, -1, -1, 0, 0, GetLocMsg(MSG_PLUGIN_NAME), NULL, NULL, NULL, NULL, MenuItems, ARRAY_SIZE(MenuItems));
			
			if (nMItem == -1)
				return nullptr;
			else if (nMItem == 1) // Show modules selection dialog
			{
				int nSelectedModItem = SelectModuleToOpenFileAs();
				if (nSelectedModItem >= 0)
					nOpenModuleIndex = nSelectedModItem;
				else
					return nullptr;
			}
		}
		else
		{
			//Display message about incompatible object
		}
	}
	else if (OInfo->OpenFrom == OPEN_ANALYSE)
	{
		const OpenAnalyseInfo* AInfo = (const OpenAnalyseInfo*) OInfo->Data;
		strFullSourcePath = AInfo->Info->FileName;
		nOpenModuleIndex = (int) AInfo->Handle - 1;
	}
	else if (OInfo->OpenFrom == OPEN_SHORTCUT)
	{
		const OpenShortcutInfo* ShCutInfo = (const OpenShortcutInfo*) OInfo->Data;
		strFullSourcePath = ResolveFullPath(ShCutInfo->HostFile);
	}

	HANDLE hOpenResult = (strFullSourcePath.size() > 0) ? OpenStorage(strFullSourcePath.c_str(), false, nOpenModuleIndex) : INVALID_HANDLE_VALUE;

	if ( (hOpenResult != INVALID_HANDLE_VALUE) && (strSubPath.size() > 0) )
	{
		SetDirectoryInfo sdi = {0};
		sdi.StructSize = sizeof(SetDirectoryInfo);
		sdi.hPanel = hOpenResult;
		sdi.Dir = strSubPath.c_str();
		sdi.OpMode = OPM_SILENT;

		SetDirectoryW(&sdi);
	}

	return hOpenResult != INVALID_HANDLE_VALUE ? hOpenResult : nullptr;
}

intptr_t WINAPI GetFindDataW(GetFindDataInfo* fdInfo)
{
	StorageObject* info = (StorageObject *) fdInfo->hPanel;
	if (!info || !info->CurrentDir()) return FALSE;

	size_t nTotalItems = info->CurrentDir()->GetChildCount();
	fdInfo->ItemsNumber = nTotalItems;

	// Zero items - exit now
	if (nTotalItems == 0) return TRUE;

	fdInfo->PanelItem = (PluginPanelItem *) malloc(nTotalItems * sizeof(PluginPanelItem));
	PluginPanelItem *panelItem = fdInfo->PanelItem;

	// Display all directories
	for (auto cit = info->CurrentDir()->subdirs.cbegin(); cit != info->CurrentDir()->subdirs.cend(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		const ContentTreeNode* node = (cit->second);

		panelItem->FileName = node->Name();
		panelItem->FileAttributes = node->GetAttributes();
		panelItem->CreationTime = node->CreationTime;
		panelItem->LastWriteTime = node->LastModificationTime;
		panelItem->FileSize = node->GetSize();
		panelItem->Owner = node->GetOwner();

		panelItem++;
	}

	// Display all files
	for (auto cit = info->CurrentDir()->files.cbegin(); cit != info->CurrentDir()->files.cend(); cit++)
	{
		memset(panelItem, 0, sizeof(PluginPanelItem));

		const ContentTreeNode* node = (cit->second);

		panelItem->FileName = node->Name();
		panelItem->FileAttributes = node->GetAttributes();
		panelItem->CreationTime = node->CreationTime;
		panelItem->LastWriteTime = node->LastModificationTime;
		panelItem->FileSize = node->GetSize();
		panelItem->Owner = node->GetOwner();

		panelItem++;
	}

	return TRUE;
}

void WINAPI FreeFindDataW(const FreeFindDataInfo* info)
{
	free(info->PanelItem);
}

intptr_t WINAPI SetDirectoryW(const SetDirectoryInfo* sdInfo)
{
	if (sdInfo->hPanel == NULL || sdInfo->hPanel == INVALID_HANDLE_VALUE)
		return FALSE;

	StorageObject* info = (StorageObject *) sdInfo->hPanel;
	if (!info) return FALSE;

	if (!sdInfo->Dir || !sdInfo->Dir[0]) return TRUE;

	return info->ChangeCurrentDir(sdInfo->Dir);
}

void WINAPI GetOpenPanelInfoW(OpenPanelInfo* opInfo)
{
	opInfo->StructSize = sizeof(OpenPanelInfo);
	
	StorageObject* info = (StorageObject *) opInfo->hPanel;
	if (!info) return;
	
	static wchar_t wszCurrentDir[PATH_BUFFER_SIZE];
	static wchar_t wszTitle[512];
	static KeyBarTitles KeyBar;
	
	static wchar_t wszSizeInfo[32];
	static wchar_t wszPackedSizeInfo[32];
	static wchar_t wszNumFileInfo[16];
	static wchar_t wszNumDirsInfo[16];
	static wchar_t wszStorageCreatedInfo[32];

	memset(wszCurrentDir, 0, sizeof(wszCurrentDir));
	memset(wszTitle, 0, sizeof(wszTitle));

	wszCurrentDir[0] = '\\';

	size_t nDirPrefixSize = wcslen(wszCurrentDir);
	info->CurrentDir()->GetPath(wszCurrentDir + nDirPrefixSize, PATH_BUFFER_SIZE - nDirPrefixSize);

	// Set title
	swprintf_s(wszTitle, ARRAY_SIZE(wszTitle), L"%s%s:%s", optPanelHeaderPrefix, info->GetModuleName(), wszCurrentDir);

	// FAR does not exit plug-in if root directory is "\"
	if (wcslen(wszCurrentDir) == 1) wszCurrentDir[0] = 0;

	I64TOW_C(info->TotalSize(), wszSizeInfo);
	I64TOW_C(info->TotalPackedSize(), wszPackedSizeInfo);

	ULTOW(info->NumFiles(), wszNumFileInfo);
	ULTOW(info->NumDirectories(), wszNumDirsInfo);

	SYSTEMTIME st;
	if (info->GeneralInfo.Created.dwHighDateTime && FileTimeToSystemTime(&info->GeneralInfo.Created, &st))
		swprintf_s(wszStorageCreatedInfo, 32, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	else
		wcscpy_s(wszStorageCreatedInfo, 32, L"-");
	
	// Fill info lines
	static InfoPanelLine pInfoLinesData[9];
	memset(pInfoLinesData, 0, sizeof(pInfoLinesData));

	pInfoLinesData[0].Text = ExtractFileName(info->StoragePath());
	pInfoLinesData[0].Flags = IPLFLAGS_SEPARATOR;
	
	pInfoLinesData[IL_FORMAT].Text = GetLocMsg(MSG_INFOL_FORMAT);
	pInfoLinesData[IL_FORMAT].Data = info->GeneralInfo.Format;

	pInfoLinesData[IL_FILES].Text = GetLocMsg(MSG_INFOL_FILES);
	pInfoLinesData[IL_FILES].Data = wszNumFileInfo;
	
	pInfoLinesData[IL_DIRECTORIES].Text = GetLocMsg(MSG_INFOL_DIRS);
	pInfoLinesData[IL_DIRECTORIES].Data = wszNumDirsInfo;

	pInfoLinesData[IL_SIZE].Text = GetLocMsg(MSG_INFOL_SIZE);
	pInfoLinesData[IL_SIZE].Data = wszSizeInfo;

	pInfoLinesData[IL_PACKED].Text = GetLocMsg(MSG_INFOL_PACKEDSIZE);
	pInfoLinesData[IL_PACKED].Data = wszPackedSizeInfo;
	
	pInfoLinesData[IL_COMPRESS].Text = GetLocMsg(MSG_INFOL_COMPRESSION);
	pInfoLinesData[IL_COMPRESS].Data = info->GeneralInfo.Compression;

	pInfoLinesData[IL_COMMENT].Text = GetLocMsg(MSG_INFOL_COMMENT);
	pInfoLinesData[IL_COMMENT].Data = info->GeneralInfo.Comment;

	pInfoLinesData[IL_CREATED].Text = GetLocMsg(MSG_INFOL_CREATED);
	pInfoLinesData[IL_CREATED].Data = wszStorageCreatedInfo;

	// Key bar customization
	static FarKey key_altF6 = {VK_F6, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED};
	static KeyBarLabel lb_altF6 = {key_altF6, GetLocMsg(MSG_ALTF6), GetLocMsg(MSG_ALTF6)};
	static KeyBarTitles kbTitles = {1, &lb_altF6};
		
	// Fill report structure
	opInfo->Flags = OPIF_ADDDOTS | OPIF_SHORTCUT;
	opInfo->CurDir = wszCurrentDir;
	opInfo->PanelTitle = wszTitle;
	opInfo->HostFile = info->StoragePath();
	opInfo->InfoLinesNumber = ARRAY_SIZE(pInfoLinesData);
	opInfo->InfoLines = pInfoLinesData;
	opInfo->KeyBar = &kbTitles;
}

intptr_t WINAPI GetFilesW(GetFilesInfo *gfInfo)
{
	if (gfInfo->Move || !gfInfo->DestPath || (gfInfo->ItemsNumber == 0))
		return 0;

	// Check for single '..' item, do not show confirm dialog
	if ((gfInfo->ItemsNumber == 1) && (wcscmp(gfInfo->PanelItem[0].FileName, L"..") == 0))
		return 0;

	StorageObject* info = (StorageObject *) gfInfo->hPanel;
	if (!info) return 0;

	ContentNodeList vcExtractItems;
	__int64 nTotalExtractSize = 0;
	int nExtNumFiles = 0, nExtNumDirs = 0;

	// Collect all indices of items to extract
	for (size_t i = 0; i < gfInfo->ItemsNumber; i++)
	{
		PluginPanelItem pItem = gfInfo->PanelItem[i];
		if (wcscmp(pItem.FileName, L"..") == 0) continue;

		ContentTreeNode* child = info->CurrentDir()->GetChildByName(pItem.FileName);
		if (child)
		{
			CollectFileList(child, vcExtractItems, nTotalExtractSize, true);

			if (pItem.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				nExtNumDirs++;
			else
				nExtNumFiles++;
		}
	} //for

	// Check if we have something to extract
	if (vcExtractItems.size() == 0)
		return 0;

	ExtractSelectedParams extParams;
	extParams.strDestPath = gfInfo->DestPath;
	extParams.bSilent = (gfInfo->OpMode & OPM_SILENT) != 0;
	extParams.bShowProgress = (gfInfo->OpMode & OPM_FIND) == 0;

	// Confirm extraction
	if (!extParams.bSilent)
	{
		IncludeTrailingPathDelim(extParams.strDestPath);
		if (!ConfirmExtract(nExtNumFiles, nExtNumDirs, extParams))
			return -1;
	}

	// Support batch operations on Shift-F2 and Shift-F3
	static ExtractSelectedParams batchParams;
	if (gfInfo->OpMode & OPM_TOPLEVEL)
	{
		if (gfInfo->OpMode & OPM_SILENT)
			extParams = batchParams;
		else
			batchParams = extParams;
	}

	return BatchExtract(info, vcExtractItems, nTotalExtractSize, extParams);
}

intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo* piInfo)
{
	if (piInfo->Rec.EventType != KEY_EVENT) return FALSE;
	
	KEY_EVENT_RECORD evtRec = piInfo->Rec.Event.KeyEvent;
	if (evtRec.bKeyDown && evtRec.wVirtualKeyCode == VK_F6 && ((evtRec.dwControlKeyState & LEFT_ALT_PRESSED) || (evtRec.dwControlKeyState & RIGHT_ALT_PRESSED)))
	{
		StorageObject* info = (StorageObject *) piInfo->hPanel;
		if (!info) return FALSE;
		
		PanelInfo pi = {sizeof(PanelInfo)};
		if (!FarSInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &pi)) return FALSE;
		if (pi.SelectedItemsNumber == 0) return FALSE;

		ContentNodeList vcExtractItems;
		__int64 nTotalExtractSize = 0;

		// Collect all indices of items to extract
		for (int i = 0; i < (int) pi.SelectedItemsNumber; i++)
		{
			intptr_t nItemBufferSize = FarSInfo.PanelControl(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, NULL);
			PluginPanelItem* pItem = (PluginPanelItem*) malloc(nItemBufferSize);
			
			FarGetPluginPanelItem gppi = { sizeof(FarGetPluginPanelItem), nItemBufferSize, pItem };
			FarSInfo.PanelControl(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, &gppi);
			if (wcscmp(pItem->FileName, L"..") != 0)
			{
				ContentTreeNode* child = info->CurrentDir()->GetChildByName(pItem->FileName);
				if (child) CollectFileList(child, vcExtractItems, nTotalExtractSize, true);
			}
			
			free(pItem);
		} //for

		// Check if we have something to extract
		if (vcExtractItems.size() == 0) return TRUE;

		wchar_t *wszTargetDir = _wcsdup(info->StoragePath());
		CutFileNameFromPath(wszTargetDir, true);
		
		ExtractSelectedParams extParams;
		extParams.strDestPath = wszTargetDir;
		extParams.bSilent = true;

		BatchExtract(info, vcExtractItems, nTotalExtractSize, extParams);

		free(wszTargetDir);
		
		return TRUE;
	}

	return FALSE;
}
