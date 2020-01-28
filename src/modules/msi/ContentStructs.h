#ifndef _CONTENT_STRUCTS_H_
#define _CONTENT_STRUCTS_H_

#include "PackageStruct.h"

#define MAX_SHORT_NAME_LEN 14
const FILETIME ZERO_FILE_TIME = {0};

class BasicNode
{
private:
	BasicNode(const BasicNode &node) = delete;
	BasicNode &operator=(const BasicNode &a) = delete;

public:
	std::wstring Key;
	BasicNode* Parent;
	DWORD Attributes;

	std::wstring SourceName;
	std::wstring SourceShortName;
	std::wstring TargetName;
	std::wstring TargetShortName;

	FILETIME ftCreationTime;
	FILETIME ftModificationTime;

public:
	BasicNode();

	virtual DWORD GetSytemAttributes() = 0;
	virtual std::wstring GetSourcePath() = 0;
	virtual std::wstring GetTargetPath() = 0;
	virtual __int64 GetSize() const = 0;

	bool IsDir() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) > 0; };
};

class FileNode : public BasicNode
{
private:
	FileNode(const FileNode &node) = delete;
	FileNode &operator=(const FileNode &a) = delete;

public:
	std::wstring Component;
	INT32 FileSize;
	int SequenceMark;
	
	bool IsFake;
	char* FakeFileContent;

public:
	FileNode();
	~FileNode();

	bool Init(FileEntry *entry);

	DWORD GetSytemAttributes();
	std::wstring GetSourcePath();
	std::wstring GetTargetPath();
	__int64 GetSize() const { return FileSize; };
};

class DirectoryNode : public BasicNode
{
private:
	DirectoryNode(const DirectoryNode &node) = delete;
	DirectoryNode &operator=(const DirectoryNode &a) = delete;

public:
	std::wstring ParentKey;
	bool IsSpecial;

	std::vector<DirectoryNode*> SubDirs;
	std::vector<FileNode*> Files;

	DirectoryNode();
	~DirectoryNode();

	bool Init(DirectoryEntry *entry, bool substDotTargetWithSource);
	bool Init(const std::wstring& dirName);
	void AddSubdir(DirectoryNode *child);
	void AddFile(FileNode *file) { Files.push_back(file); file->Parent = this; }

	int GetFilesCount();
	int GetSubDirsCount();
	__int64 GetTotalSize();

	DWORD GetSytemAttributes();
	std::wstring GetSourcePath();
	std::wstring GetTargetPath();
	__int64 GetSize() const { return 0; };
};

#endif //_CONTENT_STRUCTS_H_
