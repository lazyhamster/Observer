#ifndef InterfaceCommon_h__
#define InterfaceCommon_h__

#include "ContentStructures.h"
#include "FarStorage.h"

#ifndef NO_CHECK_FAR_VERSION
	#ifndef FARMANAGERVERSION_MAJOR
	#error Major version of FAR can not be found
	#endif
#endif

enum KeepPathValues
{
	KPV_NOPATH = 0,
	KPV_FULLPATH = 1,
	KPV_PARTIAL = 2
};

struct ProgressContext
{
#if FARMANAGERVERSION_MAJOR == 1
	char szFilePath[MAX_PATH];
#else
	wchar_t wszFilePath[MAX_PATH];
#endif
	int nCurrentFileNumber;
	__int64 nCurrentFileSize;

	int nTotalFiles;
	__int64 nTotalSize;

	__int64 nProcessedFileBytes;
	__int64 nTotalProcessedBytes;
	int nCurrentProgress;

	bool bDisplayOnScreen;

	ProgressContext()
	{
#if FARMANAGERVERSION_MAJOR == 1
		memset(szFilePath, 0, sizeof(szFilePath));
#else
		memset(wszFilePath, 0, sizeof(wszFilePath));
#endif
		nCurrentFileNumber = 0;
		nTotalFiles = 0;
		nTotalSize = 0;
		nProcessedFileBytes = 0;
		nCurrentFileSize = 0;
		nTotalProcessedBytes = 0;
		nCurrentProgress = -1;
		bDisplayOnScreen = true;
	}
};

struct ExtractSelectedParams
{
#if FARMANAGERVERSION_MAJOR == 1
	string strDestPath;
#else
	wstring strDestPath;
#endif
	int nPathProcessing;			// from KeepPathValues
	int nOverwriteExistingFiles;    // 0 - skip, 1 - overwrite, 2 - ask
	bool bShowProgress;
	bool bSilent;

#if FARMANAGERVERSION_MAJOR == 1
	ExtractSelectedParams(const char* dstPath)
#else	
	ExtractSelectedParams(const wchar_t* dstPath)
#endif
	{
		strDestPath = dstPath;
		nPathProcessing = KPV_PARTIAL;
		nOverwriteExistingFiles = 2;
		bShowProgress = true;
		bSilent = false;
	}
};

enum InfoLines
{
	IL_FORMAT = 1,
	IL_FILES = 2,
	IL_DIRECTORIES = 3,
	IL_SIZE = 4,
	IL_PACKED = 5,
	IL_COMPRESS = 6,
	IL_COMMENT = 7,
	IL_CREATED = 8,
	IL_LAST = 9
};

class SaveConsoleTitle
{
private:
	wchar_t m_OldTitle[512];

	SaveConsoleTitle() { /* hidden */ }

public:
	SaveConsoleTitle(const wchar_t *newTitle) { memset(m_OldTitle, 0, sizeof(m_OldTitle)); SetNewTitle(newTitle); }
	~SaveConsoleTitle() { Restore(); }

	void SetNewTitle(const wchar_t* newTitle)
	{
		GetConsoleTitle(m_OldTitle, ARRAY_SIZE(m_OldTitle));
		SetConsoleTitle(newTitle);
	}

	void Restore()
	{
		if (m_OldTitle[0])
		{
			SetConsoleTitle(m_OldTitle);
			m_OldTitle[0] = 0;
		}
	}
};

// Extract error reactions
#define EEN_ABORT 1
#define EEN_RETRY 2
#define EEN_SKIP 3
#define EEN_SKIPALL 4
#define EEN_CONTINUE 5
#define EEN_CONTINUESILENT 6

#define ULTOW(num, wstr) _ultow_s(num, wstr, ARRAY_SIZE(wstr), 10);

#define I64TOW_C(num, wstr) _i64tow_s(num, wstr, ARRAY_SIZE(wstr), 10); InsertCommas(wstr);

int CollectFileList(ContentTreeNode* node, ContentNodeList &targetlist, __int64 &totalSize, bool recursive);
wstring GetFinalExtractionPath(const StorageObject* storage, const ContentTreeNode* item, const wchar_t* baseDir, int keepPathOpt);

#endif // InterfaceCommon_h__
