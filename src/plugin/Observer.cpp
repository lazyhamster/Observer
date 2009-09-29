// Observer.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <far/plugin.hpp>

#include "ModulesController.h"
#include "ContentStructures.h"
#include "PlugLang.h"

#define PATH_BUFFER_SIZE 4096

extern HMODULE g_hDllHandle;
PluginStartupInfo FarSInfo;
static FarStandardFunctions FSF;

wchar_t wszPluginLocation[MAX_PATH];
ModulesController g_pController;

#define CP_FAR_INTERNAL CP_OEMCP

// Settings
#define MAX_PREFIX_SIZE 32
static int optEnabled = TRUE;
static int optUsePrefix = TRUE;
static char optPrefix[MAX_PREFIX_SIZE] = "observe";

struct FarStorageInfo
{
	int ModuleIndex;
	INT_PTR *StoragePtr;
	wchar_t StorageFileName[MAX_PATH];
	StorageGeneralInfo gi;
	ContentTreeNode* items;			// All pre-allocated items, array, must be deleted
	ContentTreeNode* root;			// First in items list, do not delete
	ContentTreeNode* currentdir;	// Just pointer, do not delete
};

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
	string strOptKey(FarSInfo.RootKey);
	strOptKey.append("\\Observer");

	HKEY reg;
	if (RegOpenKeyA(HKEY_CURRENT_USER, strOptKey.c_str(), &reg) != ERROR_SUCCESS) return;

	DWORD dwValueSize;
	RegQueryValueExA(reg, "Enabled", NULL, NULL, (LPBYTE) &optEnabled, &(dwValueSize = sizeof(optEnabled)));
	RegQueryValueExA(reg, "UsePrefix", NULL, NULL, (LPBYTE) &optUsePrefix, &(dwValueSize = sizeof(optPrefix)));
	RegQueryValueExA(reg, "Prefix", NULL, NULL, (LPBYTE) &optPrefix, &(dwValueSize = MAX_PREFIX_SIZE));

	RegCloseKey(reg);
}

static void SaveSettings()
{
	string strOptKey(FarSInfo.RootKey);
	strOptKey.append("\\Observer");

	HKEY reg;
	if (RegCreateKeyA(HKEY_CURRENT_USER, strOptKey.c_str(), &reg) != ERROR_SUCCESS) return;

	RegSetValueExA(reg, "Enabled", 0, REG_DWORD, (BYTE *) &optEnabled, sizeof(optEnabled));
	RegSetValueExA(reg, "UsePrefix", 0, REG_DWORD, (BYTE *) &optUsePrefix, sizeof(optUsePrefix));
	RegSetValueExA(reg, "Prefix", 0, REG_SZ, (BYTE *) &optPrefix, strlen(optPrefix) + 1);
	
	RegCloseKey(reg);
}

static bool CheckEsc()
{
	DWORD dwNumEvents;
	_INPUT_RECORD inRec;

	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
	if (GetNumberOfConsoleInputEvents(hConsole, &dwNumEvents))
		while (PeekConsoleInputA(hConsole, &inRec, 1, &dwNumEvents) && (dwNumEvents > 0))
		{
			ReadConsoleInputA(hConsole, &inRec, 1, &dwNumEvents);
			if ((inRec.EventType == KEY_EVENT) && (inRec.Event.KeyEvent.bKeyDown) && (inRec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
				return true;
		} //while

	return false;
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

static void DisplayMessage(bool isError, bool isInteractive, const char* header, const char* errorText, const char* errorItem)
{
	static const char* MsgLines[3];
	MsgLines[0] = header;
	MsgLines[1] = errorText;
	MsgLines[2] = errorItem;

	int linesNum = (errorItem) ? 3 : 2;
	int flags = 0;
	if (isError) flags |= FMSG_WARNING;
	if (isInteractive) flags |= FMSG_MB_OK;
	
	FarSInfo.Message(FarSInfo.ModuleNumber, flags, NULL, MsgLines, linesNum, 0);
}

static bool FileExists(const wchar_t* path)
{
	WIN32_FIND_DATAW fdata;
	
	HANDLE sr = FindFirstFileW(path, &fdata);
	if (sr != INVALID_HANDLE_VALUE)
	{
		FindClose(sr);
		return true;
	}

	return false;
}

//-----------------------------------  Content functions ----------------------------------------

HANDLE OpenStorage(const wchar_t* Name)
{
	INT_PTR *storage = NULL;
	StorageGeneralInfo sinfo;
	memset(&sinfo, 0, sizeof(sinfo));

	int moduleIndex;
	if (!g_pController.OpenStorageFile(Name, &moduleIndex, &storage, &sinfo))
		return INVALID_HANDLE_VALUE;

	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (storage != NULL)
	{
		HANDLE hScreen = FarSInfo.SaveScreen(0, 0, -1, -1);
		DisplayMessage(false, false, GetLocMsg(MSG_PLUGIN_NAME), GetLocMsg(MSG_OPEN_LIST), NULL);
		
		bool fListOK = true;
		DWORD nNumItems = sinfo.NumRealItems;
		ContentTreeNode* all_items = new ContentTreeNode[nNumItems + 1];  // +1 for root
		
		ContentTreeNode* root_node = &all_items[0];

		wchar_t* wszItemPathBuf = new wchar_t[PATH_BUFFER_SIZE];
		for (DWORD item_index = 0; item_index < nNumItems; item_index++)
		{
			ContentTreeNode* child = &(all_items[item_index + 1]);
			child->storageIndex = item_index;

			if (g_pController.modules[moduleIndex].GetNextItem(storage, item_index, &(child->data), wszItemPathBuf, PATH_BUFFER_SIZE))
			{
				if (!root_node->AddChild(wszItemPathBuf, child))
				{
					DisplayMessage(true, true, GetLocMsg(MSG_OPEN_CONTENT_ERROR), GetLocMsg(MSG_OPEN_INVALID_ITEM), NULL);
					fListOK = false;
					break;
				}
			}
			else
			{
				DisplayMessage(true, true, GetLocMsg(MSG_OPEN_CONTENT_ERROR), GetLocMsg(MSG_OPEN_INVALID_ITEM), NULL);
				fListOK = false;
				break;
			}
		} //for
		delete [] wszItemPathBuf;
		
		if (fListOK)
		{
			FarStorageInfo *info = new FarStorageInfo;
			info->ModuleIndex = moduleIndex;
			info->StoragePtr = storage;
			info->items = all_items;
			info->currentdir = root_node;
			info->root = root_node;
			info->gi = sinfo;
			
			// Copy storage file name for info
			const wchar_t *slashPos = wcsrchr(Name, L'\\');
			if (slashPos)
				wcscpy_s(info->StorageFileName, MAX_PATH, slashPos+1);
			else
				wcscpy_s(info->StorageFileName, MAX_PATH, Name);

			hResult = (HANDLE) info;
		}
		else
		{
			delete [] all_items;
		}

		FarSInfo.RestoreScreen(hScreen);
	}

	return hResult;
}

HANDLE OpenStorageA(const char* Name)
{
	wchar_t *wszWideName = new wchar_t[PATH_BUFFER_SIZE];
	wmemset(wszWideName, 0, PATH_BUFFER_SIZE);
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, Name, strlen(Name), wszWideName, PATH_BUFFER_SIZE);

	HANDLE hResult = OpenStorage(wszWideName);

	delete [] wszWideName;
	return hResult;
}

void CloseStorage(HANDLE storage)
{
	FarStorageInfo *info = (FarStorageInfo *) storage;
	if (info->ModuleIndex < (int) g_pController.modules.size())
	{
		g_pController.CloseStorageFile(info->ModuleIndex, info->StoragePtr);
		delete [] info->items;
	}
	delete info;
}

//-----------------------------------  Callback functions ----------------------------------------

static char szLastExtractName[PATH_BUFFER_SIZE] = {0};

static void ExtractStart(HANDLE *context)
{
	if (context)
		*context = FarSInfo.SaveScreen(0, 0, -1, -1);
	
	// Shring file path to fit on console
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hStdOut, &si))
		FSF.TruncPathStr(szLastExtractName, si.dwSize.X - 16);

	static const char* InfoLines[3];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_EXTRACTING);
	InfoLines[2] = szLastExtractName;

	FarSInfo.Message(FarSInfo.ModuleNumber, 0, NULL, InfoLines, sizeof(InfoLines) / sizeof(InfoLines[0]), 0);
}

static void ExtractDone(HANDLE context)
{
	if (context)
		FarSInfo.RestoreScreen(context);
}

static int ExtractError(int errorReason, HANDLE context)
{
	static const char* InfoLines[6];
	InfoLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	InfoLines[1] = GetLocMsg(MSG_EXTRACT_FAILED);
	InfoLines[2] = szLastExtractName;
	InfoLines[3] = "Retry";
	InfoLines[4] = "Skip";
	InfoLines[5] = "Abort";

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

int CALLBACK ExtractProgress(int progressValue, HANDLE context)
{
	// Check for ESC pressed
	if (CheckEsc())
		return FALSE;
	
	//TODO: show progress bar (progress report not implemented in modules for now)
	return TRUE;
}

//-----------------------------------  Extract functions ----------------------------------------

// Overwrite values
#define EXTR_OVERWRITE_ASK 1
#define EXTR_OVERWRITE_SILENT 2
#define EXTR_OVERWRITE_SKIP 3
#define EXTR_OVERWRITE_SKIPSILENT 4

bool AskExtractOverwrite(int &overwrite)
{
	static const char* DlgLines[8];
	DlgLines[0] = GetLocMsg(MSG_PLUGIN_NAME);
	DlgLines[1] = GetLocMsg(MSG_EXTRACT_OVERWRITE);
	DlgLines[2] = szLastExtractName;
	DlgLines[3] = "&Overwrite";
	DlgLines[4] = "Overwrite &all";
	DlgLines[5] = "&Skip";
	DlgLines[6] = "S&kip all";
	DlgLines[7] = "&Cancel";

	int nMsg = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_WARNING, NULL, DlgLines, sizeof(DlgLines) / sizeof(DlgLines[0]), 5);
	
	if (nMsg == 5)
		return false;
	else
	{
		overwrite = nMsg + 1;
		return true;
	}
}

static int ExtractStorageItem(FarStorageInfo* storage, ContentTreeNode* item, const wchar_t* destDir, bool recursive, bool silent, int &doOverwrite)
{
	if (!item || !storage) return FALSE;

	// Check for ESC pressed
	if (CheckEsc())	return FALSE;

	if (item->IsDir())
	{
		// Create destination dir
		wstring strDir(destDir);
		strDir.append(item->data.cFileName);
		strDir.append(L"\\");

		if (!CreateDirectoryW(strDir.c_str(), NULL))
		{
			DisplayMessage(true, true, GetLocMsg(MSG_EXTRACT_ERROR), GetLocMsg(MSG_EXTRACT_DIR_CREATE_ERROR), NULL);
			return FALSE;
		}
		SetFileAttributesW(strDir.c_str(), item->data.dwFileAttributes);

		if (recursive)
		{
			// Iterate through sub directories
			for (SubNodesMap::const_iterator cit = item->subdirs.begin(); cit != item->subdirs.end(); cit++)
			{
				ContentTreeNode* child = cit->second;
				if (!ExtractStorageItem(storage, child, strDir.c_str(), recursive, silent, doOverwrite))
					return FALSE;
			} //for
		}
		
		// Iterate through files
		for (SubNodesMap::const_iterator cit = item->files.begin(); cit != item->files.end(); cit++)
		{
			ContentTreeNode* child = cit->second;
			if (!ExtractStorageItem(storage, child, strDir.c_str(), recursive, silent, doOverwrite))
				return FALSE;
		} //for
	}
	else
	{
		wstring strFullTargetPath(destDir);
		strFullTargetPath += item->data.cFileName;

		static wchar_t wszNextItemSubPath[PATH_BUFFER_SIZE];
		item->GetPath(wszNextItemSubPath, PATH_BUFFER_SIZE);

		// Save file name for dialogs
		memset(szLastExtractName, 0, PATH_BUFFER_SIZE);
		WideCharToMultiByte(CP_FAR_INTERNAL, 0, wszNextItemSubPath, wcslen(wszNextItemSubPath), szLastExtractName, PATH_BUFFER_SIZE, NULL, NULL);

		// Ask about overwrite if needed
		if (!silent && FileExists(strFullTargetPath.c_str()))
		{
			if (doOverwrite == EXTR_OVERWRITE_ASK)
				if (!AskExtractOverwrite(doOverwrite))
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

		HANDLE ctx;
		ExtractStart(&ctx);
		
		// Set callbacks
		ExtractProcessCallbacks sig;
		sig.Progress = ExtractProgress;
		sig.signalContext = ctx;
			
		int ret;
		do
		{
			ExtractOperationParams params;
			params.item = wszNextItemSubPath;
			params.Params = 0;
			params.destPath = destDir;
			params.Callbacks = sig;

			ret = g_pController.modules[storage->ModuleIndex].Extract(storage->StoragePtr, params);

			if ((ret == SER_ERROR_WRITE) || (ret == SER_ERROR_READ))
			{
				int errorResp = silent ? EEN_ABORT : ExtractError(ret, ctx);
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

		} while ((ret != SER_SUCCESS) && (ret != SER_ERROR_SYSTEM) && (ret != SER_USERABORT));

		ExtractDone(ctx);
		return (ret == SER_SUCCESS);
	}
	
	return TRUE;
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
		DI_BUTTON,    0,6,  0, 7, 0, 0, DIF_CENTERGROUP,0,"Cancel"
	};
	FarDialogItem DialogItems[sizeof(InitItems)/sizeof(InitItems[0])];

	InitDialogItems(InitItems,DialogItems, sizeof(InitItems)/sizeof(InitItems[0]));

	DialogItems[1].Selected = optEnabled;
	DialogItems[2].Selected = optUsePrefix;
	strcpy_s(DialogItems[3].Data, sizeof(DialogItems[3].Data) / sizeof(DialogItems[3].Data[0]), optPrefix);

	int ExitCode = FarSInfo.Dialog(FarSInfo.ModuleNumber, -1, -1, 40, 9, "TempCfg", DialogItems, sizeof(DialogItems)/sizeof(DialogItems[0]));

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
	FarStorageInfo *info = (FarStorageInfo *) hPlugin;
	if (info->ModuleIndex < (int) g_pController.modules.size())
	{
		g_pController.modules[info->ModuleIndex].CloseStorage(info->StoragePtr);
		delete [] info->items;
	}
	delete info;
}

HANDLE WINAPI OpenPlugin(int OpenFrom, INT_PTR Item)
{
	// Unload plugin if no submodules loaded
	if (g_pController.modules.size() == 0)
		return 0;
	
	char* szFullNameBuffer = new char[PATH_BUFFER_SIZE];
	DWORD fpres = 0;
	
	if ((OpenFrom == OPEN_COMMANDLINE) && optUsePrefix)
	{
		char* szLocalNameBuffer = new char[PATH_BUFFER_SIZE];

		strcpy_s(szLocalNameBuffer, MAX_PATH, (char *) Item);
		FSF.ExpandEnvironmentStr(szLocalNameBuffer, szLocalNameBuffer, PATH_BUFFER_SIZE);
		fpres = GetFullPathNameA(szLocalNameBuffer, PATH_BUFFER_SIZE, szFullNameBuffer, NULL);

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

	HANDLE hOpenResult = (fpres && (fpres < PATH_BUFFER_SIZE)) ? OpenStorageA(szFullNameBuffer) : INVALID_HANDLE_VALUE;

	delete [] szFullNameBuffer;
	return hOpenResult;
}

HANDLE WINAPI OpenFilePlugin(char *Name, const unsigned char *Data, int DataSize)
{
	if (!Name || !optEnabled)
		return INVALID_HANDLE_VALUE;

	HANDLE hOpenResult = OpenStorageA(Name);
	return hOpenResult;
}

int WINAPI GetFindData(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	FarStorageInfo* info = (FarStorageInfo *) hPlugin;
	if (!info || !info->currentdir) return FALSE;

	int nTotalItems = info->currentdir->GetChildCount();
	*pItemsNumber = nTotalItems;

	// Zero items - exit now
	if (nTotalItems == 0) return TRUE;

	*pPanelItem = (PluginPanelItem *) malloc(nTotalItems * sizeof(PluginPanelItem));
	PluginPanelItem *panelItem = *pPanelItem;

	// Display all directories
	for (SubNodesMap::const_iterator cit = info->currentdir->subdirs.begin(); cit != info->currentdir->subdirs.end(); cit++)
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
	for (SubNodesMap::const_iterator cit = info->currentdir->files.begin(); cit != info->currentdir->files.end(); cit++)
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

	return TRUE;
}

void WINAPI FreeFindData(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
	free(PanelItem);
}

int WINAPI SetDirectory(HANDLE hPlugin, const char *Dir, int OpMode)
{
	if (hPlugin == NULL)
		return FALSE;

	FarStorageInfo* info = (FarStorageInfo *) hPlugin;
	if (!info) return FALSE;

	if (strcmp(Dir, "..") == 0)
	{
		if (info->currentdir->parent == NULL)
			return FALSE;

		info->currentdir = info->currentdir->parent;
	}
	else if (strcmp(Dir, ".") != 0)
	{
		wchar_t *wzDirName = new wchar_t[MAX_PATH];
		wmemset(wzDirName, 0, MAX_PATH);
		MultiByteToWideChar(CP_FAR_INTERNAL, 0, Dir, strlen(Dir), wzDirName, MAX_PATH);

		ContentTreeNode* child_dir = info->currentdir->GetSubDir(wzDirName);
		delete [] wzDirName;

		if (!child_dir)	return FALSE;
		info->currentdir = child_dir;
	}
		
	return TRUE;
}

void WINAPI GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
	FarStorageInfo* info = (FarStorageInfo *) hPlugin;
	if (!info) return;
	
	static char szCurrentDir[MAX_PATH];
	static char szTitle[1024];
	static wchar_t wszCurrentDirPath[PATH_BUFFER_SIZE];

	memset(szCurrentDir, 0, sizeof(szCurrentDir));
	memset(szTitle, 0, sizeof(szTitle));
	memset(wszCurrentDirPath, 0, sizeof(wszCurrentDirPath));

	info->currentdir->GetPath(wszCurrentDirPath, PATH_BUFFER_SIZE);
	WideCharToMultiByte(CP_FAR_INTERNAL, 0, wszCurrentDirPath, wcslen(wszCurrentDirPath), szCurrentDir, MAX_PATH, NULL, NULL);

	WideCharToMultiByte(CP_FAR_INTERNAL, 0, g_pController.modules[info->ModuleIndex].ModuleName, wcslen(g_pController.modules[info->ModuleIndex].ModuleName), szTitle, 1024, NULL, NULL);
	strcat_s(szTitle, 1024, ":\\");
	strcat_s(szTitle, 1024, szCurrentDir);
	
	Info->StructSize = sizeof(OpenPluginInfo);
	Info->Flags = OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE;
	Info->CurDir = szCurrentDir;
	Info->PanelTitle = szTitle;

	// Fill info lines
	static InfoPanelLine pInfoLinesData[5];
	size_t nInfoTextSize = sizeof(pInfoLinesData[0].Text) / sizeof(pInfoLinesData[0].Text[0]);
	size_t nInfoDataSize = sizeof(pInfoLinesData[0].Data) / sizeof(pInfoLinesData[0].Data[0]);

	memset(pInfoLinesData, 0, sizeof(pInfoLinesData));
	WideCharToMultiByte(CP_ACP, 0, info->StorageFileName, wcslen(info->StorageFileName), pInfoLinesData[0].Text, nInfoTextSize, NULL, NULL);
	pInfoLinesData[0].Separator = 1;
	
	strcpy_s(pInfoLinesData[1].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_FORMAT));
	WideCharToMultiByte(CP_ACP, 0, info->gi.Format, wcslen(info->gi.Format), pInfoLinesData[1].Data, nInfoDataSize, NULL, NULL);

	strcpy_s(pInfoLinesData[2].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_FILES));
	_ultoa_s(info->gi.NumFiles, pInfoLinesData[2].Data, nInfoDataSize, 10);

	strcpy_s(pInfoLinesData[3].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_DIRS));
	_ultoa_s(info->gi.NumDirectories, pInfoLinesData[3].Data, nInfoDataSize, 10);

	strcpy_s(pInfoLinesData[4].Text, nInfoTextSize, GetLocMsg(MSG_INFOL_SIZE));
	_i64toa_s(info->gi.TotalSize, pInfoLinesData[4].Data, nInfoDataSize, 10);
	InsertCommas(pInfoLinesData[4].Data);
	
	Info->InfoLinesNumber = sizeof(pInfoLinesData) / sizeof(pInfoLinesData[0]);
	Info->InfoLines = pInfoLinesData;
}


int WINAPI GetFiles(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	if (Move || !DestPath || (OpMode & OPM_FIND) || !ItemsNumber)
		return 0;

	// Check for single '..' item, do not show confirm dialog
	if ((ItemsNumber == 1) && (strcmp(PanelItem[0].FindData.cFileName, "..") == 0))
		return 0;

	FarStorageInfo* info = (FarStorageInfo *) hPlugin;
	if (!info) return 0;

	// Confirm extraction
	if ((OpMode & OPM_SILENT) == 0)
	{
		static const char* ConfirmBox[2];
		ConfirmBox[0] = GetLocMsg(MSG_PLUGIN_NAME);
		ConfirmBox[1] = GetLocMsg(MSG_EXTRACT_CONFIRM);
		int choise = FarSInfo.Message(FarSInfo.ModuleNumber, FMSG_MB_OKCANCEL, NULL, ConfirmBox, 2, 2);
		if (choise != 0) return -1;
	}

	// Prepare destination path
	size_t nDestPathLen = strlen(DestPath);
	wchar_t *wszWideDestPath = (wchar_t *) malloc((nDestPathLen + 2) * sizeof(wchar_t));
	wmemset(wszWideDestPath, 0, nDestPathLen + 2);
	MultiByteToWideChar(CP_FAR_INTERNAL, 0, DestPath, nDestPathLen, wszWideDestPath, nDestPathLen + 2);
	if (wszWideDestPath[nDestPathLen - 1] != '\\')
		wcscat_s(wszWideDestPath, nDestPathLen + 2, L"\\");

	SHCreateDirectory(0, wszWideDestPath);

	int nExtractResult = TRUE;
	wchar_t wszItemNameBuf[MAX_PATH] = {0};
	int doOverwrite = EXTR_OVERWRITE_ASK;

	//vector<int> vcExtractItems;

	for (int i = 0; i < ItemsNumber; i++)
	{
		PluginPanelItem pItem = PanelItem[i];
		if (strcmp(pItem.FindData.cFileName, "..") == 0) continue;

		wmemset(wszItemNameBuf, 0, MAX_PATH);
		MultiByteToWideChar(CP_FAR_INTERNAL, 0, pItem.FindData.cFileName, strlen(pItem.FindData.cFileName), wszItemNameBuf, MAX_PATH);

		ContentTreeNode* child = info->currentdir->GetChildByName(wszItemNameBuf);
		if (child)
		{
			nExtractResult = ExtractStorageItem(info, child, wszWideDestPath, true, (OpMode & OPM_SILENT) > 0, doOverwrite);
			if (!nExtractResult) break;
		}
	} //for

	free(wszWideDestPath);

	//TODO: add recursion option
	//TODO: add option to keep full path
	return nExtractResult;
}
