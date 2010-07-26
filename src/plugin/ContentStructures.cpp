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

size_t ContentTreeNode::GetPath(wchar_t* dest, size_t destSize, ContentTreeNode* upRoot) const
{
	if (!data.cFileName[0])
	{
		if (dest) *dest = 0;
		return 0;
	}
	
	size_t ret = 0;
	if (parent && (parent != upRoot))
	{
		ret = parent->GetPath(dest, destSize, upRoot);
		if (ret > 0)
		{
			if (dest != NULL)
				wcscat_s(dest, destSize, L"\\");
			ret++;
		}
		if (dest != NULL)
			wcscat_s(dest, destSize, data.cFileName);
	}
	else
	{
		if (dest != NULL)
			wcscpy_s(dest, destSize, data.cFileName);
	}

	ret += wcslen(data.cFileName);
	return ret;
}

bool ContentTreeNode::AddChild(wchar_t* path, ContentTreeNode* child)
{
	wchar_t* slash = wcschr(path, '\\');
	if (slash)
	{
		ContentTreeNode* sub_dir = NULL;
				
		*slash = 0;
		if (wcscmp(path, L".") == 0)
			sub_dir = this;
		else if (wcscmp(path, L"..") == 0)
			sub_dir = this->parent;
		else
		{
			sub_dir = GetSubDir(path);
		
			if (!sub_dir)
				sub_dir = insertDummyDirectory(path);
		}

		if (!sub_dir) return false;
		
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
	// If name is not NULL but empty or equal to \ return self
	if (name && (!*name || !wcscmp(name, L"\\")))
		return this;
	
	const wchar_t* slash = wcschr(name, L'\\');
	if (slash != NULL)
	{
		size_t nSliceSize = slash - name;
		if (nSliceSize > 0)
		{
			wchar_t* tmpName = new wchar_t[nSliceSize + 1];
			wcsncpy_s(tmpName, nSliceSize + 1, name, nSliceSize);

			ContentTreeNode* child = GetSubDir(tmpName);
			delete [] tmpName;
			
			if (child)
				return child->GetChildByName(slash + 1);
		}
	}
	else
	{
		ContentTreeNode* child = GetSubDir(name);
		if (child) return child;

		SubNodesMap::iterator it = files.find(name);
		return (it == files.end()) ? NULL : (*it).second;
	}

	return NULL;
}

size_t ContentTreeNode::GetSubDirectoriesNum( bool recursive )
{
	size_t nres = subdirs.size();
	if (recursive)
	{
		for (SubNodesMap::const_iterator citer = subdirs.begin(); citer != subdirs.end(); citer++)
			nres += citer->second->GetSubDirectoriesNum(recursive);
	}
	return nres;
}
