#include "StdAfx.h"
#include "ContentStructures.h"

/*
static int GetStrHash(const wchar_t* src, size_t max_len = -1)
{
	if (!src) return 0;
	
	int h = 0;
	size_t i = 0;
	while (src[i] && ((i < max_len) || (max_len == -1)))
	{
		h += h * 31 + src[i];
		i++;
	}

	return h;
}
*/

//////////////////////////////////////////////////////

ContentTreeNode::ContentTreeNode()
{
	parent = NULL;
	storageIndex = -1;
	memset(&data, 0, sizeof(data));
}

ContentTreeNode::~ContentTreeNode()
{
	if (m_dummyFolders.size() > 0)
		for (vector<ContentTreeNode*>::iterator iter = m_dummyFolders.begin(); iter != m_dummyFolders.end(); iter++)
		{
			ContentTreeNode* dir = *iter;
			delete dir;
		}

	//subitems.clear();
}

void ContentTreeNode::GetPath(wchar_t* dest, size_t destSize) const
{
	if (!data.cFileName[0])
	{
		*dest = 0;
		return;
	}
	
	if (parent)
	{
		parent->GetPath(dest, destSize);
		if (*dest) wcscat_s(dest, destSize, L"\\");
		wcscat_s(dest, destSize, data.cFileName);
	}
	else
	{
		wcscpy_s(dest, destSize, data.cFileName);
	}
}

bool ContentTreeNode::AddChild(wchar_t* path, ContentTreeNode* child)
{
	wchar_t* slash = wcschr(path, '\\');
	if (slash)
	{
		ContentTreeNode* sub_dir = NULL;
				
		*slash = 0;
		sub_dir = GetSubDir(path);
		if (!sub_dir)
			sub_dir = insertDummyDirectory(path);
		
		*slash = '\\';
		while (*slash == '\\') // This cycle solves issues with double backslashes in path
			slash++;

		return sub_dir->AddChild(slash, child);
	}
	else
	{
		SubNodesMap &targetMap = child->IsDir() ? subdirs : files;
		
		// Check for duplicates
		if (targetMap.find(path) != targetMap.end())
			return false;

		child->parent = this;
		targetMap.insert(pair<wstring, ContentTreeNode* > (child->data.cFileName, child));
	}

	return true;
}

ContentTreeNode* ContentTreeNode::insertDummyDirectory(const wchar_t *name)
{
	ContentTreeNode* dummyDir = new ContentTreeNode();
	
	dummyDir->parent = this;
	dummyDir->storageIndex = -1;
	wcscpy_s(dummyDir->data.cFileName, MAX_PATH, name);
	dummyDir->data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

	this->subdirs.insert(pair<wstring, ContentTreeNode*> (name, dummyDir));
	m_dummyFolders.push_back(dummyDir);
	
	return dummyDir;
}

ContentTreeNode* ContentTreeNode::GetSubDir(const wchar_t* name)
{
	SubNodesMap::iterator it = subdirs.find(name);
	if (it == subdirs.end())
		return NULL;
	else
		return (*it).second;
}

ContentTreeNode* ContentTreeNode::GetChildByName(const wchar_t* name)
{
	ContentTreeNode* child = GetSubDir(name);
	if (child) return child;

	SubNodesMap::iterator it = files.find(name);
	return (it == files.end()) ? NULL : (*it).second;
}
