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
	wstring strDestPath;
	int nPathProcessing;			// from KeepPathValues
	int nOverwriteExistingFiles;    // 0 - skip, 1 - overwrite, 2 - ask
	bool bShowProgress;
	bool bSilent;

	ExtractSelectedParams(const wchar_t* dstPath)
	{
		strDestPath = dstPath;
		nPathProcessing = KPV_PARTIAL;
		nOverwriteExistingFiles = 2;
		bShowProgress = true;
		bSilent = false;
	}
};

int CollectFileList(ContentTreeNode* node, ContentNodeList &targetlist, __int64 &totalSize, bool recursive);
wstring GetFinalExtractionPath(const StorageObject* storage, const ContentTreeNode* item, const wchar_t* baseDir, int keepPathOpt);

#endif // InterfaceCommon_h__
