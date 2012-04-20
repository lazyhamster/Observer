#ifndef wcx_loader_h__
#define wcx_loader_h__

#include "wcxhead.h"

typedef HANDLE (__stdcall *OpenArchiveFunc) (tOpenArchiveData *ArchiveData);
typedef int (__stdcall *ReadHeaderFunc) (HANDLE hArcData, tHeaderData *HeaderData);
typedef int (__stdcall *ReadHeaderExFunc) (HANDLE hArcData, tHeaderDataEx *HeaderDataEx);
typedef int (__stdcall *ProcessFileFunc) (HANDLE hArcData, int Operation, char *DestPath, char *DestName);
typedef void (__stdcall *SetProcessDataProcFunc) (HANDLE hArcData, tProcessDataProc pProcessDataProc);
typedef BOOL (__stdcall *CanYouHandleThisFileFunc) (char *FileName);

typedef HANDLE (__stdcall *OpenArchiveWFunc) (tOpenArchiveDataW *ArchiveData);
typedef int (__stdcall *ReadHeaderExWFunc) (HANDLE hArcData, tHeaderDataExW *HeaderData);
typedef int (__stdcall *ProcessFileWFunc) (HANDLE hArcData, int Operation, wchar_t *DestPath, wchar_t *DestName);
typedef void (__stdcall *SetProcessDataProcWFunc) (HANDLE hArcData, tProcessDataProcW pProcessDataProc);
typedef BOOL (__stdcall *CanYouHandleThisFileWFunc) (wchar_t *FileName);

typedef int (__stdcall *CloseArchiveFunc) (HANDLE hArcData);

struct WcxModule
{
	HMODULE ModuleHandle;

	// Unicode functions
	CanYouHandleThisFileWFunc CanYouHandleThisFileW;
	OpenArchiveWFunc OpenArchiveW;
	ReadHeaderExWFunc ReadHeaderExW;
	ProcessFileWFunc ProcessFileW;
	SetProcessDataProcWFunc SetProcessDataProcW;

	// Ansi functions
	CanYouHandleThisFileFunc CanYouHandleThisFile;
	OpenArchiveFunc OpenArchive;
	ReadHeaderFunc ReadHeader;
	ReadHeaderExFunc ReadHeaderEx;
	ProcessFileFunc ProcessFile;
	SetProcessDataProcFunc SetProcessDataProc;

	// Common functions
	CloseArchiveFunc CloseArchive;
};

#endif // wcx_loader_h__
