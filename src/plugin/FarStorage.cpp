#include "StdAfx.h"
#include "FarStorage.h"
#include "CommonFunc.h"

StorageObject::StorageObject( ModulesController *modules, PasswordQueryCallbackFunc PassCallback )
{
	m_pModules = modules;
	
	m_nModuleIndex = -1;
	m_pStoragePtr = NULL;
	m_wszStoragePath = NULL;

	m_pRootDir = NULL;
	m_pCurrentDir = NULL;

	m_nTotalSize = 0;
	m_nTotalPackedSize = 0;
	m_nNumFiles = 0;
	m_nNumDirectories = 0;

	m_fnPassCallback = PassCallback;

	memset(&GeneralInfo, 0, sizeof(GeneralInfo));
}

StorageObject::~StorageObject()
{
	Close();
}

bool StorageObject::Open( const wchar_t* path, bool applyExtFilters, int openWithModule )
{
	return Open(path, nullptr, 0, applyExtFilters, openWithModule);
}

bool StorageObject::Open( const wchar_t* path, const void* data, size_t dataSize, bool applyExtFilters, int openWithModule )
{
	Close();
	
	int moduleIndex = 0;
	HANDLE storagePtr = NULL;
	char passBuf[100] = {0};

	OpenStorageFileInParams srcParams = {0};
	srcParams.path = path;
	srcParams.applyExtFilters = applyExtFilters;
	srcParams.openWithModule = openWithModule;
	srcParams.dataBuffer = data;
	srcParams.dataSize = dataSize;
	
	int retVal = m_pModules->OpenStorageFile(srcParams, &moduleIndex, &storagePtr, &GeneralInfo);
	
	// If some module requested password, then try to request it from user and try to open file again
	while (retVal == SOR_PASSWORD_REQUIRED && m_fnPassCallback != NULL)
	{
		if ( ! m_fnPassCallback(passBuf, ARRAY_SIZE(passBuf)) )
			break;

		srcParams.password = passBuf;
		srcParams.openWithModule = moduleIndex;
		retVal = m_pModules->OpenStorageFile(srcParams, &moduleIndex, &storagePtr, &GeneralInfo);
	} // while

	if (retVal == SOR_SUCCESS)
	{
		m_nModuleIndex = moduleIndex;
		m_pStoragePtr = storagePtr;
		m_wszStoragePath = _wcsdup(srcParams.path);
		
		m_pRootDir = new ContentTreeNode();
		m_pCurrentDir = NULL;

		return true;
	}

	return false;
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
	}

	m_pCurrentDir = NULL;
	m_nTotalSize = 0;
	m_nTotalPackedSize;
	m_nNumFiles = 0;
	m_nNumDirectories = 0;

	// Release all nodes
	for (auto iter = m_vItems.begin(); iter != m_vItems.end(); iter++)
	{
		ContentTreeNode* node = *iter;
		delete node;
	}
	m_vItems.clear();
}

bool StorageObject::ReadFileList(bool &aborted)
{
	bool fListOK = true;
	aborted = false;

	__int64 nTotalSize = 0, nTotalPackedSize = 0;
	DWORD nNumFiles = 0, nNumDirs = 0;

	const ExternalModule &module = m_pModules->GetModule(m_nModuleIndex);
		
	DWORD item_index = 0;
	StorageItemInfo item_info;

	if (!module.ModuleFunctions.PrepareFiles(m_pStoragePtr))
		return false;

	do
	{
		memset(&item_info, 0, sizeof(item_info));
		int res = module.ModuleFunctions.GetItem(m_pStoragePtr, item_index, &item_info);

		if (res == GET_ITEM_NOMOREITEMS)
		{
			// No more items, just exit successfully
			break;
		}
		else if (res == GET_ITEM_OK)
		{
			ContentTreeNode* child = new ContentTreeNode(item_index, &item_info);
			
			if (m_pRootDir->AddChild(item_info.Path, child))
			{
				m_vItems.push_back(child);
				if (!child->IsDir())
				{
					nNumFiles++;
					nTotalSize += child->GetSize();
					nTotalPackedSize += child->GetPackedSize();
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

	if (fListOK)
	{
		m_pCurrentDir = m_pRootDir;

		m_nTotalSize = nTotalSize;
		m_nTotalPackedSize = nTotalPackedSize;
		m_nNumFiles = nNumFiles;
		m_nNumDirectories = nNumDirs;

		// Calculate number of sub-directories if module does not supply ones
		if (m_nNumDirectories == 0)
			m_nNumDirectories = (int) m_pRootDir->GetSubDirectoriesNum(true);

		return true;
	}
	
	return false;
}

int StorageObject::Extract( ExtractOperationParams &params )
{
	const ExternalModule &module = m_pModules->GetModule(m_nModuleIndex);
	return module.ModuleFunctions.ExtractItem(m_pStoragePtr, params);
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
