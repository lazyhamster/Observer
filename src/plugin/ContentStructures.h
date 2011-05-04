#ifndef _CONTENT_STRUCTURES_H_
#define _CONTENT_STRUCTURES_H_

#include "ModuleDef.h"

class ContentTreeNode;
typedef map<wstring, ContentTreeNode*> SubNodesMap;
typedef vector<ContentTreeNode*> ContentNodeList;

class ContentTreeNode
{
private:
	// Support for storages where only files are defined (with path though)
	vector<ContentTreeNode* > m_dummyFolders;
	ContentTreeNode* insertDummyDirectory(const wchar_t *name);

	ContentTreeNode* GetSubDir(const wchar_t* name);
	void AddFile(ContentTreeNode* child);
	void Init(int item_index, StorageItemInfo* item_info);

	wstring m_strName;
	__int64 m_nSize;

public:
	int StorageIndex;
	DWORD Attributes;
	FILETIME LastModificationTime;
	FILETIME CreationTime;

	ContentTreeNode* parent;
	SubNodesMap subdirs;
	SubNodesMap files;

	ContentTreeNode();
	ContentTreeNode(int index, StorageItemInfo* info);
	~ContentTreeNode();

	size_t GetPath(wchar_t* dest, size_t destSize, ContentTreeNode* upRoot = NULL) const;
	bool AddChild(wchar_t* path, ContentTreeNode* child);

	size_t GetChildCount() { return subdirs.size() + files.size(); };
	ContentTreeNode* GetChildByName(const wchar_t* name);
	bool IsDir() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) > 0; }

	__int64 Size() const {return IsDir() ? 0 : m_nSize; };
	const wchar_t* Name() const { return m_strName.c_str(); }

	size_t GetSubDirectoriesNum(bool recursive);
};

#endif