#ifndef CommonFunc_h__
#define CommonFunc_h__

#include "ModuleDef.h"
#include "ContentStructures.h"

#define PATH_BUFFER_SIZE 4096

bool FileExists(const wchar_t* path, LPWIN32_FIND_DATAW file_data);
bool DirectoryExists(const wchar_t* path);
bool IsDiskRoot(const wchar_t* path);
bool CheckEsc();
int CollectFileList(ContentTreeNode* node, vector<int> &targetlist, __int64 &totalSize, bool recursive);
bool IsEnoughSpaceInPath(const wchar_t* path, __int64 requiredSize);
bool ForceDirectoryExist(const wchar_t* path);
const wchar_t* ExtractFileName(const wchar_t* fullPath);

#endif // CommonFunc_h__