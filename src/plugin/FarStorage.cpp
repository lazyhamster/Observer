#include "StdAfx.h"
#include "FarStorage.h"

StorageObject::StorageObject( ModulesController *modules )
{
	m_pModules = modules;
	
	m_nModuleIndex = -1;
	m_pStoragePtr = NULL;
	m_wszStoragePath = NULL;

	m_pRootDir = NULL;
	m_pCurrentDir = NULL;

	m_nTotalSize = 0;
	m_nNumFiles = 0;
	m_nNumDirectories = 0;

	memset(&GeneralInfo, 0, sizeof(GeneralInfo));
}

StorageObject::~StorageObject()
{
	Close();
}

int StorageObject::Open( const wchar_t *Path )
{
	Close();
	
	int moduleIndex = 0;
	INT_PTR *storagePtr = NULL;
	
	int ret = m_pModules->OpenStorageFile(Path, &moduleIndex, &storagePtr, &GeneralInfo);
	if (ret)
	{
		m_nModuleIndex = moduleIndex;
		m_pStoragePtr = storagePtr;
		m_wszStoragePath = _wcsdup(Path);
		
		m_pRootDir = new ContentTreeNode();
		m_pCurrentDir = NULL;
	}

	return ret;
}

void StorageObject::Close()
{
	if (m_pStoragePtr)
	{
		m_pModules->CloseStorageFile(m_nModuleIndex, m_pStoragePtr);
		m_pStoragePtr = NULL;
		m_nModuleIndex = -1;
	}
	if (m_wszStoragePath)
	{
		free(m_wszStoragePath);
		m_wszStoragePath = NULL;
	}

	if (m_pRootDir)
	{
		delete m_pRootDir;
		m_pRootDir = NULL;
		m_pCurrentDir = NULL;
	}

	m_nTotalSize = 0;
	m_nNumFiles = 0;
	m_nNumDirectories = 0;

	// Release all nodes
	vector<ContentTreeNode*>::iterator iter;
	for (iter = m_vItems.begin(); iter != m_vItems.end(); iter++)
	{
		ContentTreeNode* node = *iter;
		delete node;
	}
	m_vItems.clear();
}

int StorageObject::ReadFileList(bool &aborted)
{
	bool fListOK = true;
	aborted = false;

	__int64 nTotalSize = 0;
	DWORD nNumFiles = 0, nNumDirs = 0;

	size_t nItemPathBufSize = PATH_BUFFER_SIZE;
	wchar_t* wszItemPathBuf = (wchar_t*) malloc(nItemPathBufSize * sizeof(wchar_t));

	ExternalModule &module = m_pModules->modules[m_nModuleIndex];
		
	DWORD item_index = 0;
	WIN32_FIND_DATAW child_data;
	do
	{
		memset(&child_data, 0, sizeof(child_data));
		int res = module.GetNextItem(m_pStoragePtr, item_index, &child_data, wszItemPathBuf, nItemPathBufSize);

		if (res == GET_ITEM_NOMOREITEMS)
		{
			// No more items, just exit successfully
			break;
		}
		else if (res == GET_ITEM_OK)
		{
			ContentTreeNode* child = new ContentTreeNode();
			child->storageIndex = item_index;
			memcpy(&(child->data), &child_data, sizeof(child_data));
			
			if (m_pRootDir->AddChild(wszItemPathBuf, child))
			{
				m_vItems.push_back(child);
				if (!child->IsDir())
				{
					nNumFiles++;
					nTotalSize += child->GetSize();
				}
				else
				{
					nNumDirs++;
				}
			}
			else
			{
				fListOK = false;
				delete child;
			}
		}
		else // Any error
		{
			fListOK = false;
		}

		// Check for user abort
		if (CheckEsc())
		{
			fListOK = false;
			aborted = true;
		}

		item_index++;

	} while (fListOK);

	free(wszItemPathBuf);

	if (fListOK)
	{
		m_pCurrentDir = m_pRootDir;

		m_nTotalSize = nTotalSize;
		m_nNumFiles = nNumFiles;
		m_nNumDirectories = nNumDirs;

		return TRUE;
	}
	
	return FALSE;
}

int StorageObject::Extract( ExtractOperationParams &params )
{
	return m_pModules->modules[m_nModuleIndex].Extract(m_pStoragePtr, params);
}

int StorageObject::ChangeCurrentDir( const wchar_t* path )
{
	if (!path || !path[0]) return FALSE;
	
	wchar_t delim[] = L"\\";
	wchar_t *path_str = _wcsdup(path);

	// Normalize delimiters
	for (size_t i = 0; i < wcslen(path_str); i++)
	{
		if (path_str[i] == '/') path_str[i] = '\\';
	}

	ContentTreeNode* baseDir = (path_str[0] == '\\') ? m_pRootDir : m_pCurrentDir;
	if (baseDir == NULL) return FALSE;
	
	wchar_t* context = NULL;
	wchar_t* token = wcstok_s(path_str, delim, &context);
	while (token)
	{
		if (wcslen(token) > 0)
		{
			if (wcscmp(token, L"..") == 0)
			{
				baseDir = baseDir->parent;
				if (!baseDir) break;
			}
			else if (wcscmp(token, L".") == 0)
			{
				// Do nothing, stay in same directory
			}
			else
			{
				baseDir = baseDir->GetChildByName(token);
				if (!baseDir) break;
			}
		}

		token = wcstok_s(NULL, delim, &context);
	}
	free(path_str);

	if (baseDir)
	{
		m_pCurrentDir = baseDir;
		return TRUE;
	}

	return FALSE;
}

ContentTreeNode* StorageObject::GetItem( size_t index )
{
	if ((index >= 0) && (index < m_vItems.size()))
		return m_vItems[index];
	else
		return NULL;
}
