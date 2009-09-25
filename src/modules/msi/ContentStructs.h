#ifndef _CONTENT_STRUCTS_H_
#define _CONTENT_STRUCTS_H_

#include "PackageStruct.h"

#define MAX_SHORT_NAME_LEN 14

class DirectoryNode;

class FileNode
{
public:
	wchar_t* Key;
	DirectoryNode* Parent;
	INT32 FileSize;
	DWORD FileAttributes;
	int SequenceMark;
	
	wchar_t* SourceName;
	wchar_t* SourceShortName;
	wchar_t* TargetName;
	wchar_t* TargetShortName;

	bool IsFake;
	char* FakeFileContent;

	int SequentialIndex;

public:
	FileNode();
	~FileNode();

	bool Init(FileEntry *entry);

	DWORD GetSytemAttributes();
	wstring GetSourcePath();
	wstring GetTargetPath();
};

class DirectoryNode
{
private:
	DirectoryNode(const DirectoryNode &node) {};

public:
	wchar_t* Key;
	wchar_t* ParentKey;
	DirectoryNode* Parent;
			
	wchar_t* SourceName;
	wchar_t* SourceShortName;
	wchar_t* TargetName;
	wchar_t* TargetShortName;
	bool IsSpecial;

	int SequentialIndex;
	
	vector<DirectoryNode*> SubDirs;
	vector<FileNode*> Files;

	DirectoryNode();
	~DirectoryNode();

	bool Init(DirectoryEntry *entry, bool substDotTargetWithSource);
	void AddSubdir(DirectoryNode *child);
	void AddFile(FileNode *file) { Files.push_back(file); file->Parent = this; }

	int GetFilesCount();
	int GetSubDirsCount();
	__int64 GetTotalSize();

	DWORD GetSytemAttributes();
	wstring GetSourcePath();
	wstring GetTargetPath();
	void AppendNumberToName(int val);
};

#endif //_CONTENT_STRUCTS_H_
