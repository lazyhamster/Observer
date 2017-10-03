#ifndef InterfaceCommon_h__
#define InterfaceCommon_h__

#include "ContentStructures.h"
#include "FarStorage.h"

#define CONFIG_FILE L"observer.ini"
#define CONFIG_USER_FILE L"observer_user.ini"

enum KeepPathValues
{
	KPV_NOPATH = 0,
	KPV_FULLPATH = 1,
	KPV_PARTIAL = 2
};

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

	bool bDisplayOnScreen;

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
		bDisplayOnScreen = true;
	}
};

struct ExtractSelectedParams
{
	std::wstring strDestPath;
	int nPathProcessing;			// from KeepPathValues
	int nOverwriteExistingFiles;    // 0 - skip, 1 - overwrite, 2 - ask
	bool bShowProgress;
	bool bSilent;

	ExtractSelectedParams()
	{
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
