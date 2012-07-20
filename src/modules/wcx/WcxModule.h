#ifndef WcxModule_h__
#define WcxModule_h__

// WCX functions prototypes

#include "wcxhead.h"

typedef HANDLE (__stdcall *OpenArchiveFunc) (tOpenArchiveData *ArchiveData);
typedef HANDLE (__stdcall *OpenArchiveWFunc) (tOpenArchiveDataW *ArchiveData);

typedef int (__stdcall *ReadHeaderFunc) (HANDLE hArcData, tHeaderData *HeaderData);
typedef int (__stdcall *ReadHeaderExFunc) (HANDLE hArcData, tHeaderDataEx *HeaderDataEx);
typedef int (__stdcall *ReadHeaderExWFunc) (HANDLE hArcData, tHeaderDataExW *HeaderData);

typedef int (__stdcall *ProcessFileFunc) (HANDLE hArcData, int Operation, char *DestPath, char *DestName);
typedef int (__stdcall *ProcessFileWFunc) (HANDLE hArcData, int Operation, wchar_t *DestPath, wchar_t *DestName);

typedef void (__stdcall *SetProcessDataProcFunc) (HANDLE hArcData, tProcessDataProc pProcessDataProc);
typedef void (__stdcall *SetProcessDataProcWFunc) (HANDLE hArcData, tProcessDataProcW pProcessDataProc);

typedef void (__stdcall *SetChangeVolProcFunc) (HANDLE hArcData, tChangeVolProc pChangeVolProc1);
typedef void (__stdcall *SetChangeVolProcWFunc) (HANDLE hArcData, tChangeVolProcW pChangeVolProc1);

typedef BOOL (__stdcall *CanYouHandleThisFileFunc) (const char *FileName);
typedef BOOL (__stdcall *CanYouHandleThisFileWFunc) (const wchar_t *FileName);

typedef int (__stdcall *CloseArchiveFunc) (HANDLE hArcData);

class IWcxModule
{
protected:
	HMODULE m_Module;
	std::wstring m_ModuleName;
	int m_DefaultCodePage;

	CloseArchiveFunc modCloseArchive;

	virtual bool InternalInit(HMODULE module) = 0;

public:
	bool Load(HMODULE module, const wchar_t* moduleName, int codePage);
	void Unload();

	const wchar_t* GetModuleName() { return m_ModuleName.c_str(); }

	virtual bool IsArchive(const wchar_t* wszFilePath) = 0;
	virtual HANDLE OpenArchive(const wchar_t* wszFilePath, int nOpMode) = 0;
	virtual int ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData) = 0;
	virtual int ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName) = 0;
	
	void CloseArchive(HANDLE hArchive) { modCloseArchive(hArchive); }
};

class WcxUnicodeModule : public IWcxModule
{
protected:
	CanYouHandleThisFileWFunc modCanYouHandleThisFile;
	OpenArchiveWFunc modOpenArchive;
	ReadHeaderExWFunc modReadHeader;
	ProcessFileWFunc modProcessFile;
	SetProcessDataProcWFunc modSetProcessDataProc;
	SetChangeVolProcWFunc modSetChangeVolProc;
	
	bool InternalInit(HMODULE module);

	static int __stdcall ProcessDataProcW(wchar_t *FileName,int Size) { return TRUE; }

public:
	bool IsArchive(const wchar_t* wszFilePath);
	HANDLE OpenArchive(const wchar_t* wszFilePath, int nOpMode);
	int ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData);
	int ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName);
};

class WcxAnsiModule : public IWcxModule
{
protected:
	CanYouHandleThisFileFunc modCanYouHandleThisFile;
	OpenArchiveFunc modOpenArchive;
	ReadHeaderFunc modReadHeader;
	ProcessFileFunc modProcessFile;
	SetProcessDataProcFunc modSetProcessDataProc;
	SetChangeVolProcFunc modSetChangeVolProc;
	
	bool InternalInit(HMODULE module);

	static int __stdcall ProcessDataProc(char *FileName,int Size) { return TRUE; }

public:
	bool IsArchive(const wchar_t* wszFilePath);
	HANDLE OpenArchive(const wchar_t* wszFilePath, int nOpMode);
	int ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData);
	int ProcessFile(HANDLE hArchive, int Operation, wchar_t *DestPath, wchar_t *DestName);
};

class WcxAnsiExtModule : public WcxAnsiModule
{
protected:
	ReadHeaderExFunc modReadHeaderEx;
	
	bool InternalInit(HMODULE module);

public:
	int ReadHeader(HANDLE hArchive, tHeaderDataExW *HeaderData);
};

#endif // WcxModule_h__
