#ifndef CommonFunc_h__
#define CommonFunc_h__

#include "ModuleDef.h"
#include "ContentStructures.h"

#define PATH_BUFFER_SIZE 4096

struct StorageInfo
{
	StorageGeneralInfo GeneralInfo;

	__int64 TotalSize;
	DWORD NumFiles;
	DWORD NumDirectories;
};

struct FarStorageInfo
{
	int ModuleIndex;
	INT_PTR *StoragePtr;
	wchar_t *StoragePath;
	StorageInfo info;
	ContentTreeNode* items;			// All pre-allocated items, array, must be deleted
	ContentTreeNode* root;			// First in items list, do not delete
	ContentTreeNode* currentdir;	// Just pointer, do not delete
};

bool FileExists(const wchar_t* path, LPWIN32_FIND_DATAW file_data);
bool DirectoryExists(const wchar_t* path);
bool IsDiskRoot(const wchar_t* path);
bool CheckEsc();
int CollectFileList(ContentTreeNode* node, vector<int> &targetlist, __int64 &totalSize, bool recursive);
bool IsEnoughSpaceInPath(const wchar_t* path, __int64 requiredSize);

#endif // CommonFunc_h__