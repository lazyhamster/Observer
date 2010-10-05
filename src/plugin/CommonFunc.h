#ifndef CommonFunc_h__
#define CommonFunc_h__

#include "ContentStructures.h"

#define PATH_BUFFER_SIZE 4096

bool FileExists(const wchar_t* path, LPWIN32_FIND_DATAW file_data);
bool DirectoryExists(const wchar_t* path);
bool IsDiskRoot(const wchar_t* path);
bool IsEnoughSpaceInPath(const wchar_t* path, __int64 requiredSize);
bool ForceDirectoryExist(const wchar_t* path);

bool CheckEsc();

int CollectFileList(ContentTreeNode* node, ContentNodeList &targetlist, __int64 &totalSize, bool recursive);

const wchar_t* ExtractFileName(const wchar_t* fullPath);
wstring GetDirectoryName(const wstring &fullPath, bool includeTrailingDelim);

#endif // CommonFunc_h__