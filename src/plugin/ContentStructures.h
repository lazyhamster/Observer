#ifndef _CONTENT_STRUCTURES_H_
#define _CONTENT_STRUCTURES_H_

class ContentTreeNode;
typedef map<wstring, ContentTreeNode*> SubNodesMap;

class ContentTreeNode
{
private:
	// Support for storages where only files are defined (with path though)
	vector<ContentTreeNode* > m_dummyFolders;
	ContentTreeNode* insertDummyDirectory(const wchar_t *name);

	ContentTreeNode* GetSubDir(const wchar_t* name);

public:
	int storageIndex;
	WIN32_FIND_DATAW data;
	ContentTreeNode* parent;
	SubNodesMap subdirs;
	SubNodesMap files;

	ContentTreeNode();
	~ContentTreeNode();

	size_t GetPath(wchar_t* dest, size_t destSize, ContentTreeNode* upRoot = NULL) const;
	bool AddChild(wchar_t* path, ContentTreeNode* child);

	size_t GetChildCount() { return subdirs.size() + files.size(); };
	ContentTreeNode* GetChildByName(const wchar_t* name);
	bool IsDir() { return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0; }
	__int64 GetSize() {return IsDir() ? 0 : (data.nFileSizeLow + ((__int64)(data.nFileSizeHigh) << 32)); };

	size_t GetSubDirectoriesNum(bool recursive);
};

#endif