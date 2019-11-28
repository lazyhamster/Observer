#include "StdAfx.h"
#include "ContentStructures.h"
#include "CommonFunc.h"

ContentTreeNode::ContentTreeNode()
{
	StorageItemInfo nullInfo = {0};
	Init(-1, &nullInfo);
}

ContentTreeNode::ContentTreeNode( int index, StorageItemInfo* info )
{
	Init(index, info);
}

ContentTreeNode::~ContentTreeNode()
{
	for (auto iter = m_dummyFolders.begin(); iter != m_dummyFolders.end(); iter++)
	{
		ContentTreeNode* dir = *iter;
		delete dir;
	}
	m_dummyFolders.clear();

	//subitems.clear();
}

void ContentTreeNode::Init( int item_index, StorageItemInfo* item_info )
{
	parent = NULL;
	StorageIndex = item_index;
	m_strName = ExtractFileName(item_info->Path);
	m_nSize = item_info->Size;
	m_nPackedSize = item_info->PackedSize;
	m_nAttributes = item_info->Attributes;
	m_nNumberOfHardlinks = item_info->NumHardlinks;
	m_strOwner = item_info->Owner;
	LastModificationTime = item_info->ModificationTime;
	CreationTime = item_info->CreationTime;
}

size_t ContentTreeNode::GetPath(wchar_t* dest, size_t destSize, ContentTreeNode* upRoot) const
{
	if (m_strName.length() == 0)
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
			wcscat_s(dest, destSize, Name());
	}
	else
	{
		if (dest != NULL)
			wcscpy_s(dest, destSize, Name());
	}

	ret += m_strName.length();
	return ret;
}

std::wstring ContentTreeNode::GetPath(ContentTreeNode* upRoot) const
{
	if (m_strName.length() == 0)
		return L"";

	if (parent && (parent != upRoot))
	{
		std::wstring strUp = parent->GetPath(upRoot);
		if (strUp.length() > 0) strUp += L"\\";
		strUp += Name();

		return strUp;
	}

	return m_strName;
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
		child->parent = this;
		if (child->IsDir())
		{
			if (!GetSubDir(path))
				subdirs.insert(std::pair<std::wstring, ContentTreeNode* > (child->Name(), child));
		}
		else
		{
			AddFile(child);
		}
	}

	return true;
}

ContentTreeNode* ContentTreeNode::insertDummyDirectory(const wchar_t *name)
{
	ContentTreeNode* dummyDir = new ContentTreeNode();
	
	dummyDir->parent = this;
	dummyDir->StorageIndex = -1;
	dummyDir->m_strName = name;
	dummyDir->m_nAttributes = FILE_ATTRIBUTE_DIRECTORY;

	this->subdirs.insert(std::pair<std::wstring, ContentTreeNode*> (name, dummyDir));
	m_dummyFolders.push_back(dummyDir);
	
	return dummyDir;
}

ContentTreeNode* ContentTreeNode::GetSubDir(const wchar_t* name)
{
	auto it = subdirs.find(name);
	if (it == subdirs.end())
		return NULL;
	else
		return (*it).second;
}

ContentTreeNode* ContentTreeNode::GetChildByName(const wchar_t* name)
{
	if (!name) return nullptr;
	
	// If name is not NULL but empty or equal to \ return self
	if (!*name || (wcscmp(name, L"\\") == 0))
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

		auto it = files.find(name);
		if (it != files.end())
			return (*it).second;
	}

	return nullptr;
}

size_t ContentTreeNode::GetSubDirectoriesNum( bool recursive )
{
	size_t nres = subdirs.size();
	if (recursive)
	{
		for (auto citer = subdirs.cbegin(); citer != subdirs.cend(); citer++)
			nres += citer->second->GetSubDirectoriesNum(recursive);
	}
	return nres;
}

void ContentTreeNode::AddFile( ContentTreeNode* child )
{
	// If file with same name already exists in the directory then append number to name
	if (files.find(child->Name()) != files.end())
	{
		wchar_t nameBuf[MAX_PATH] = {0};
		wcscpy_s(nameBuf, ARRAY_SIZE(nameBuf), child->Name());
				
		int nDupCounter = 1;
		wchar_t tmpBuf[MAX_PATH];
		wchar_t* extPtr = wcsrchr(nameBuf, '.');

		do 
		{
			nDupCounter++;
			if (extPtr)
			{
				*extPtr = 0;
				swprintf_s(tmpBuf, ARRAY_SIZE(tmpBuf), L"%s,%d.%s", nameBuf, nDupCounter, extPtr+1);
			}
			else
			{
				swprintf_s(tmpBuf, ARRAY_SIZE(tmpBuf), L"%s,%d", nameBuf, nDupCounter);
			}
		} while (files.find(tmpBuf) != files.end());

		child->SetName(tmpBuf);
	} //if

	files.insert(std::pair<std::wstring, ContentTreeNode*> (child->Name(), child));
}
