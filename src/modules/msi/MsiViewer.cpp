#include "StdAfx.h"
#include "MsiViewer.h"

#pragma comment(lib, "Msi.lib")

#define OK(f) res=f; \
	if(res!=ERROR_SUCCESS) return res;
#define OK_MISS(f) res=f; \
	if(res == ERROR_BAD_QUERY_SYNTAX) return ERROR_SUCCESS; \
	else if(res != ERROR_SUCCESS) return res;
#define READ_STR(rec,ind,s) nCellSize = sizeof(s)/sizeof(s[0]); OK( MsiRecordGetStringW(rec, ind, s, &nCellSize) );

#define FCOPY_BUF_SIZE 32*1024

//TODO: display features in tree structure

struct MsiSpecialFoderDesc
{
	wchar_t* Name;
	wchar_t* ShortName;
};

MsiSpecialFoderDesc MsiSpecialFolders[] = {
	{L"AdminToolsFolder", L"AdminT~1"},			//The full path to the directory that contains administrative tools.
	{L"AppDataFolder", L"AppDat~1"},			//The full path to the Roaming folder for the current user. 
	{L"CommonAppDataFolder", L"Common~1"},		//The full path to application data for all users.
	{L"CommonFiles64Folder", L"Common~2"},		//The full path to the predefined 64-bit Common Files folder.
	{L"CommonFilesFolder", L"Common~3"},		//The full path to the Common Files folder for the current user.
	{L"DesktopFolder", L"Deskto~1"},			//The full path to the Desktop folder.
	{L"FavoritesFolder", L"Favori~1"},			//The full path to the Favorites folder for the current user.
	{L"FontsFolder", L"FontsF~1"},				//The full path to the Fonts folder.
	{L"LocalAppDataFolder", L"LocalA~1"},		//The full path to the folder that contains local (nonroaming) applications. 
	{L"MyPicturesFolder", L"MyPict~1"},			//The full path to the Pictures folder.
	{L"PersonalFolder", L"Person~1"},			//The full path to the Documents folder for the current user.
	{L"ProgramFiles64Folder", L"Progra~1"},		//The full path to the predefined 64-bit Program Files folder.
	{L"ProgramFilesFolder", L"Progra~2"},		//The full path to the predefined 32-bit Program Files folder.
	{L"ProgramMenuFolder", L"Progra~3"},		//The full path to the Program Menu folder.
	{L"SendToFolder", L"SendTo~1"},				//The full path to the SendTo folder for the current user.
	{L"StartMenuFolder", L"StartM~1"},			//The full path to the Start menu folder.
	{L"StartupFolder", L"Startu~1"},			//The full path to the Startup folder.
	{L"System16Folder", L"System~1"},			//The full path to folder for 16-bit system DLLs.
	{L"System64Folder", L"System~2"},			//The full path to the predefined System64 folder.
	{L"SystemFolder", L"System~3"},				//The full path to the System folder for the current user.
	{L"TempFolder", L"TempFo~1"},				//The full path to the Temp folder.
	{L"TemplateFolder", L"Templa~1"},			//The full path to the Template folder for the current user.
	{L"WindowsFolder", L"Window~1"},			//The full path to the Windows folder.
	{L"WindowsVolume", L"Window~2"},			//The volume of the Windows folder.
};

CMsiViewer::CMsiViewer(void)
{
	m_hMsi = 0;
	m_pRootDir = NULL;
	m_nSummaryWordCount = 0;
	memset(&m_ftCreateTime, 0, sizeof(m_ftCreateTime));
	m_pCabControl = new CCabControl();
}

CMsiViewer::~CMsiViewer(void)
{
	delete m_pCabControl;
	if (m_hMsi)	MsiCloseHandle(m_hMsi);
	if (m_pRootDir)	delete m_pRootDir;

	for (map<wstring, wstring>::iterator iter = m_mStreamCache.begin(); iter != m_mStreamCache.end(); iter++)
		DeleteFileW(iter->second.c_str());
	RemoveDirectoryW(m_strStreamCacheLocation.c_str());
}

int CMsiViewer::Open( const wchar_t* path, DWORD openFlags )
{
	if (m_hMsi) return -1;
	
	UINT res;
		
	OK( MsiOpenDatabaseW(path, MSIDBOPEN_READONLY, &m_hMsi) );

	DirectoryNodesMap mDirs;
	ComponentEntryMap mComponents;

	// Prepare list of media sources
	OK ( readMediaSources() );

	// Get Directory entry list
	OK ( readDirectories(mDirs) );

	// Get Component entry list
	OK ( readComponents(mDirs, mComponents) );

	// Get File entry list
	OK ( readFiles(mDirs, mComponents) );

	// Assign parent nodes (only after all entries are processed)
	// Should be done only after files are read to remove empty special folder refs
	OK ( assignParentDirs(mDirs, (openFlags & MSI_OPENFLAG_SHOWSPECIALS) != 0) );

	// Read CreateFolder table for allowed empty folders
	WStringMap mCreateFolder;
	OK ( readCreateFolder(mCreateFolder) );

	// Perform post-processing
	removeEmptyFolders(m_pRootDir, mCreateFolder);
	mergeDotFolders(m_pRootDir);
	checkShortNames(m_pRootDir);
	if ((openFlags & MSI_OPENFLAG_KEEPUNIQUEDIRS) != 0)
		avoidSameFolderNames(m_pRootDir);
	else
		mergeSameNamedFolders(m_pRootDir);

	mCreateFolder.clear();
	mComponents.clear();
	mDirs.clear();

	// Generate fake files content
	OK ( generateInfoText() );
	OK ( generateLicenseText() );

	buildFlatIndex(m_pRootDir);

	m_strStorageLocation = path;
	
	return ERROR_SUCCESS;
}

int CMsiViewer::readDirectories(DirectoryNodesMap &nodemap)
{
	nodemap.clear();

	UINT res;
	WStringMap appSearch;

	OK ( readAppSearch(appSearch) );

	if (m_pRootDir)
		delete m_pRootDir;
	m_pRootDir = new DirectoryNode();
	
	PMSIHANDLE hQueryDirs;
	OK( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Directory", &hQueryDirs) );
	OK( MsiViewExecute(hQueryDirs, 0) );

	// Retrieve all directory entries and convert to nodes
	PMSIHANDLE hDirRec;
	DirectoryEntry dirEntry;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryDirs, &hDirRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hDirRec, 1, dirEntry.Key);
		READ_STR(hDirRec, 2, dirEntry.ParentKey);
		READ_STR(hDirRec, 3, dirEntry.DefaultName);

		WStringMap::const_iterator citer = appSearch.find(dirEntry.Key);
		bool fIsAppSearch = (citer != appSearch.end());

		DirectoryNode *node = new DirectoryNode();
		if (node->Init(&dirEntry, fIsAppSearch))
			nodemap[dirEntry.Key] = node;
		else
			delete node;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::readComponents( DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap )
{
	UINT res;
	PMSIHANDLE hQueryComp;

	OK( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Component", &hQueryComp) );
	OK( MsiViewExecute(hQueryComp, 0) );

	// Retrieve all component entries
	PMSIHANDLE hCompRec;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryComp, &hCompRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		ComponentEntry compEntry;

		READ_STR(hCompRec, 1, compEntry.Key);
		READ_STR(hCompRec, 2, compEntry.ID);
		READ_STR(hCompRec, 3, compEntry.Directory);
		compEntry.Attributes = MsiRecordGetInteger(hCompRec, 4);
		if (compEntry.Attributes == MSI_NULL_INTEGER)
			return ERROR_INVALID_DATA;

		componentmap[compEntry.Key] = compEntry;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::readFiles( DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap )
{
	UINT res;
	PMSIHANDLE hQueryFile;

	OK( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM File", &hQueryFile) );
	OK( MsiViewExecute(hQueryFile, 0) );

	// Retrieve all file entries
	PMSIHANDLE hFileRec;
	FileEntry fileEntry;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryFile, &hFileRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);
		
		READ_STR(hFileRec, 1, fileEntry.Key);
		READ_STR(hFileRec, 2, fileEntry.Component);
		READ_STR(hFileRec, 3, fileEntry.FileName);
		fileEntry.FileSize = MsiRecordGetInteger(hFileRec, 4);
		fileEntry.Attributes = MsiRecordGetInteger(hFileRec, 7);
		fileEntry.Sequence = MsiRecordGetInteger(hFileRec, 8);

		if (!fileEntry.Component[0])
			return ERROR_INVALID_DATA;

		FileNode *node = new FileNode();
		node->Init(&fileEntry);

		ComponentEntry component = componentmap[fileEntry.Component];
		DirectoryNode *dir = nodemap[component.Directory];
		
		if (dir)
			dir->AddFile(node);
		else
			return ERROR_INVALID_DATA;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::readAppSearch(WStringMap &entries)
{
	UINT res;
	PMSIHANDLE hQueryAppSearch;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM AppSearch", &hQueryAppSearch) );
	OK( MsiViewExecute(hQueryAppSearch, 0) );

	wchar_t key[150];
	wchar_t signature[150];

	// Retrieve all component entries
	PMSIHANDLE hAppRec;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryAppSearch, &hAppRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hAppRec, 1, key);
		READ_STR(hAppRec, 2, signature);

		if (key[0])
			entries[key] = signature;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::readCreateFolder(WStringMap &entries)
{
	UINT res;
	PMSIHANDLE hQueryCreateFolder;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM CreateFolder", &hQueryCreateFolder) );
	OK( MsiViewExecute(hQueryCreateFolder, 0) );

	wchar_t dir[256];
	wchar_t component[256];

	// Retrieve all component entries
	PMSIHANDLE hFolderRec;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryCreateFolder, &hFolderRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hFolderRec, 1, dir);
		READ_STR(hFolderRec, 2, component);

		if (dir[0])
			entries[dir] = component;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::assignParentDirs( DirectoryNodesMap &nodemap, bool processSpecialDirs )
{
	int numSpecFolders = processSpecialDirs ? sizeof(MsiSpecialFolders) / sizeof(MsiSpecialFolders[0]) : 0;
	
	DirectoryNodesMap::iterator dirIter;
	for (dirIter = nodemap.begin(); dirIter != nodemap.end(); dirIter++)
	{
		DirectoryNode* node = dirIter->second;
		DirectoryNode* parent = (node->ParentKey) ? nodemap[node->ParentKey] : m_pRootDir;

		// This should not occur under normal circumstances
		if (!parent) return ERROR_INVALID_DATA;

		// Check for special folder
		int cmpRes = 1;
		int i = 0;
		for (i = 0; i < numSpecFolders; i++)
		{
			cmpRes = wcscmp(MsiSpecialFolders[i].Name, node->Key);
			if (cmpRes >= 0) break;
		} //while

		if (cmpRes == 0)
		{
			// Avoid merging special folders to root dir (causes confusion)
			node->IsSpecial = true;
			if (wcscmp(node->TargetName, L".") == 0)
			{
				free(node->TargetName);
				if (node->SourceName && wcscmp(node->SourceName, L"."))
					node->TargetName = _wcsdup(node->SourceName);
				else
					node->TargetName = _wcsdup(node->Key);

				free(node->TargetShortName);
				if (node->SourceShortName && wcscmp(node->SourceShortName, L"."))
					node->TargetShortName = _wcsdup(node->SourceShortName);
				else
					node->TargetShortName = _wcsdup(MsiSpecialFolders[i].ShortName);
			}
		}
		parent->AddSubdir(node);
	}

	return ERROR_SUCCESS;
}

void CMsiViewer::removeEmptyFolders(DirectoryNode *root, WStringMap &forcedFolders)
{
	if (root->SubDirs.size() == 0) return;
	
	vector<DirectoryNode*>::iterator iter;
	for (iter = root->SubDirs.begin(); iter != root->SubDirs.end();)
	{
		DirectoryNode* subDir = *iter;
		removeEmptyFolders(subDir, forcedFolders);

		WStringMap::const_iterator citer = forcedFolders.find(subDir->Key);
		bool fIsAllowedEmpty = (citer != forcedFolders.end());
		
		if ((subDir->SubDirs.size() == 0) && (subDir->Files.size() == 0) && !fIsAllowedEmpty)
		{
			iter = root->SubDirs.erase(iter);
			delete subDir;
		}
		else
			iter++;
	}
}

void CMsiViewer::mergeDotFolders( DirectoryNode *root )
{
	size_t nMaxCounter = root->SubDirs.size();
	for (size_t i = 0; i < nMaxCounter;)
	{
		DirectoryNode* subdir = root->SubDirs.at(i);
		mergeDotFolders(subdir);

		if (wcscmp(subdir->TargetName, L".") == 0)
		{
			if (subdir->SubDirs.size() > 0)
			{
				root->SubDirs.insert(root->SubDirs.end(), subdir->SubDirs.begin(), subdir->SubDirs.end());
				subdir->SubDirs.clear();

				for (size_t inner_i = 0; inner_i < root->SubDirs.size(); inner_i++)
					root->SubDirs[inner_i]->Parent = root;
			}
			if (subdir->Files.size() > 0)
			{
				root->Files.insert(root->Files.end(), subdir->Files.begin(), subdir->Files.end());
				subdir->Files.clear();

				for (size_t inner_i = 0; inner_i < root->Files.size(); inner_i++)
					root->Files[inner_i]->Parent = root;
			}

			root->SubDirs.erase(root->SubDirs.begin() + i);
			delete subdir;
			nMaxCounter--;
		}
		else
		{
			i++;
		}
	} //for
}

void CMsiViewer::avoidSameFolderNames(DirectoryNode *root)
{
	int nSameCounter;
	size_t nMaxCounter = root->SubDirs.size();
	for (size_t i = 0; i < nMaxCounter; i++)
	{
		DirectoryNode* subdir = root->SubDirs.at(i);
		
		nSameCounter = 1;
		for (size_t inner_i = i + 1; inner_i < nMaxCounter; inner_i++)
		{
			DirectoryNode* nextdir = root->SubDirs.at(inner_i);
			if (_wcsicmp(subdir->TargetName, nextdir->TargetName) == 0)
			{
				if ((nextdir->SubDirs.size() > 0) || (nextdir->Files.size() > 0))
				{
					nSameCounter++;
					nextdir->AppendNumberToName(nSameCounter);
				}
				else
				{
					root->SubDirs.erase(root->SubDirs.begin() + inner_i);
					inner_i--;
					nMaxCounter--;
					delete nextdir;
				}
			}
		}  //for

		if (nSameCounter > 1)
			subdir->AppendNumberToName(1);

		avoidSameFolderNames(subdir);
	} //for
}

void CMsiViewer::checkShortNames(DirectoryNode *root)
{
	for (vector<DirectoryNode*>::iterator iter = root->SubDirs.begin(); iter != root->SubDirs.end();)
	{
		DirectoryNode* subdir = *iter;

		if (subdir->TargetShortName && (wcslen(subdir->TargetShortName) >= MAX_SHORT_NAME_LEN))
		{
			if (subdir->GetFilesCount() == 0)
				iter = root->SubDirs.erase(iter);
			else
			{
				*(subdir->TargetShortName) = 0;
				iter++;
			}
		}
		else
		{
			checkShortNames(subdir);
			iter++;
		}
	} //for
}

void CMsiViewer::mergeSameNamedFolders(DirectoryNode *root)
{
	size_t nMaxCounter = root->SubDirs.size();
	
	// First pass to merge folders in current one
	for (size_t i = 0; i < nMaxCounter; i++)
	{
		DirectoryNode* subdir = root->SubDirs[i];
		
		for (size_t inner_i = i + 1; inner_i < nMaxCounter; inner_i++)
		{
			DirectoryNode* nextdir = root->SubDirs[inner_i];

			if (_wcsicmp(subdir->TargetName, nextdir->TargetName) == 0)
			{
				// Move subdirs from second folder to first one and update parent pointer
				subdir->SubDirs.insert(subdir->SubDirs.end(), nextdir->SubDirs.begin(), nextdir->SubDirs.end());
				for (size_t j = 0; j < subdir->SubDirs.size(); j++)
					subdir->SubDirs[j]->Parent = subdir;
				nextdir->SubDirs.clear();

				// Move files from second folder to first one and update parent pointer
				subdir->Files.insert(subdir->Files.end(), nextdir->Files.begin(), nextdir->Files.end());
				for (size_t j = 0; j < subdir->Files.size(); j++)
					subdir->Files[j]->Parent = subdir;
				nextdir->Files.clear();
				
				root->SubDirs.erase(root->SubDirs.begin() + inner_i);
				inner_i--;
				nMaxCounter--;
				delete nextdir;
			}
		} // inner for

		avoidSameFileNames(subdir);
	} //for

	// Second pass to merge everything inside sub-folders again
	for (size_t i = 0; i < nMaxCounter; i++)
	{
		DirectoryNode* subdir = root->SubDirs[i];
		mergeSameNamedFolders(subdir);
	}
}

void CMsiViewer::avoidSameFileNames(DirectoryNode *root)
{
	int nSameCounter;
	size_t nMaxCounter = root->Files.size();
	for (size_t i = 0; i < nMaxCounter; i++)
	{
		FileNode* file = root->Files[i];

		nSameCounter = 1;
		for (size_t inner_i = i + 1; inner_i < nMaxCounter; inner_i++)
		{
			FileNode* nextfile = root->Files[inner_i];
			if (_wcsicmp(file->TargetName, nextfile->TargetName) == 0)
			{
				nSameCounter++;
				nextfile->AppendNumberToName(nSameCounter);
			}
		}  //for

		if (nSameCounter > 1)
			file->AppendNumberToName(1);
	} //for
}

int CMsiViewer::generateInfoText()
{
	struct PropDescription
	{
		wchar_t* PropName;
		UINT PropID;
	};
	PropDescription SUMMARY_PROPS[] = {{L"Title", 2}, {L"Subject", 3}, {L"Author", 4},
		{L"Comments", 6}, {L"Revision Number", 9}};
	
	wstringstream sstr;
	UINT res;

	sstr << L"Installation package general info\n\n";

	// Summary info
	sstr << endl;
	sstr << L"[Summary Info]" << endl;
	
	PMSIHANDLE hSummary;
	OK ( MsiGetSummaryInformationW(m_hMsi, NULL, 0, &hSummary) );
	
	UINT nDataType;
	DWORD len;
	vector<wchar_t> propdata(512);
	for (int i = 0; i < sizeof(SUMMARY_PROPS) / sizeof(SUMMARY_PROPS[0]); i++)
	{
		while ((res = MsiSummaryInfoGetPropertyW(hSummary, SUMMARY_PROPS[i].PropID, &nDataType, NULL, NULL, &propdata[0], &(len = propdata.size()))) == ERROR_MORE_DATA)
			propdata.resize(len + 1);

		if (res == ERROR_SUCCESS)
			sstr << SUMMARY_PROPS[i].PropName << L" : " << (wchar_t *) &propdata[0] << endl;
	}

	// Read "Word Count" property to save default compression info
	int nPropVal;
	if (MsiSummaryInfoGetPropertyW(hSummary, 15, &nDataType, &nPropVal, NULL, NULL, NULL) == ERROR_SUCCESS)
		m_nSummaryWordCount = nPropVal;
	
	// Save "Create Time/Date" property for future use
	FILETIME ftPropVal;
	if (MsiSummaryInfoGetPropertyW(hSummary, 12, &nDataType, NULL, &ftPropVal, NULL, NULL) == ERROR_SUCCESS)
		m_ftCreateTime = ftPropVal;

	// Content info
	sstr << endl;
	sstr << L"Total directories: " << GetTotalDirectories() << endl;
	sstr << L"Total files: " << GetTotalFiles() << endl;

	// Features list
	sstr << endl;
	sstr << L"[Optional Features]" << endl;
	OK( dumpFeatures(sstr) );

	// Registry keys
	sstr << endl;
	sstr << L"[Registry]" << endl;
	OK ( dumpRegistryKeys(sstr) );

	wstring content = sstr.str();
	
	// Add fake file with general info to root folder
	FileNode *fake = new FileNode();
	fake->TargetName = _wcsdup(L"{msi_info}.txt");
	fake->TargetShortName = _wcsdup(L"msi_info.txt");
	fake->Attributes = 0;
	fake->IsFake = true;
	fake->FileSize = content.size() * sizeof(wchar_t) + 2;	// sizeof BOM = 2
	fake->FakeFileContent = (char *) malloc(fake->FileSize + sizeof(wchar_t)); // +1 for 0-terminator
	strcpy_s(fake->FakeFileContent, 3, "\xFF\xFE");
	memcpy_s(fake->FakeFileContent + 2, fake->FileSize - 2, content.c_str(), fake->FileSize - 2);
	m_pRootDir->AddFile(fake);

	return ERROR_SUCCESS;
}

int CMsiViewer::dumpRegistryKeys(wstringstream &sstr)
{
	UINT res;
	PMSIHANDLE hQueryReg;
	
	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Registry", &hQueryReg) );
	OK( MsiViewExecute(hQueryReg, 0) );

	// Retrieve all registry entries
	PMSIHANDLE hRegRec;
	DWORD nCellSize;
	DWORD nVlen = 0;
	vector<wchar_t> regval(512);
	RegistryEntry regEntry;
	while ((res = MsiViewFetch(hQueryReg, &hRegRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hRegRec, 1, regEntry.Key);
		regEntry.Root = MsiRecordGetInteger(hRegRec, 2);
		READ_STR(hRegRec, 3, regEntry.RegKeyName);
		READ_STR(hRegRec, 4, regEntry.Name);

		while ((res = MsiRecordGetStringW(hRegRec, 5, &regval[0], &(nVlen = regval.size()))) == ERROR_MORE_DATA)
			regval.resize(nVlen + 1);
		OK (res);

		sstr << L"[";
		switch (regEntry.Root)
		{
			case msidbRegistryRootClassesRoot:
				sstr << L"HKEY_CLASSES_ROOT";
				break;
			case msidbRegistryRootCurrentUser:
				sstr << L"HKEY_CURRENT_USER";
				break;
			case msidbRegistryRootLocalMachine:
				sstr << L"HKEY_LOCAL_MACHINE";
				break;
			case msidbRegistryRootUsers:
				sstr << L"HKEY_USERS";
				break;
			default:
				sstr << L"HKEY_CURRENT_USER";
				break;
		}
		sstr << L"\\" << regEntry.RegKeyName << L"]" << endl;
		if (regEntry.Name[0])
			sstr << L"\"" << regEntry.Name << L"\"";
		else
			sstr << L"@";
		sstr << L" = \"" << &regval[0] << L"\"" << endl;
	}
	
	return ERROR_SUCCESS;
}

int CMsiViewer::dumpFeatures(wstringstream &sstr)
{
	UINT res;
	PMSIHANDLE hQueryFeat;
	
	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Feature", &hQueryFeat) );
	OK( MsiViewExecute(hQueryFeat, 0) );

	// Retrieve all feature entries
	PMSIHANDLE hFeatRec;
	DWORD nCellSize;
	FeatureEntry featEntry;
	while ((res = MsiViewFetch(hQueryFeat, &hFeatRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hFeatRec, 1, featEntry.Key);
		READ_STR(hFeatRec, 2, featEntry.Parent);
		READ_STR(hFeatRec, 3, featEntry.Title);
		READ_STR(hFeatRec, 4, featEntry.Description);
		featEntry.Display = MsiRecordGetInteger(hFeatRec, 5);
		featEntry.Level = MsiRecordGetInteger(hFeatRec, 6);
		READ_STR(hFeatRec, 7, featEntry.Directory);

		if (featEntry.Level > 0 && featEntry.Display > 0)
		{
			if (wcslen(featEntry.Title) > 0)
				sstr << L"- " << featEntry.Title << L"\n";
			else
				sstr << L"- <" << featEntry.Key << L">\n";

			if (wcslen(featEntry.Description) > 0)
				sstr << L"    " << featEntry.Description << L"\n";
		}
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::generateLicenseText()
{
	UINT res;
	PMSIHANDLE hQueryLicense;
	
	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM `Control` WHERE `Control`.`Control` = 'AgreementText'", &hQueryLicense) );
	OK( MsiViewExecute(hQueryLicense, 0) );

	// Retrieve all registry entries
	PMSIHANDLE hLicRec;
	DWORD nVlen = 0;
	vector<char> val(512);
	while ((res = MsiViewFetch(hQueryLicense, &hLicRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		while ((res = MsiRecordGetStringA(hLicRec, 10, &val[0], &(nVlen = val.size()))) == ERROR_MORE_DATA)
			val.resize(nVlen + 1);
		
		MsiCloseHandle(hLicRec);
		OK (res); 
	}

	MsiCloseHandle(hQueryLicense);

	if (nVlen > 0)
	{
		// Add fake file with general info to root folder
		FileNode *fake = new FileNode();
		fake->TargetName = _wcsdup(L"{license}.txt");
		fake->TargetShortName = _wcsdup(L"license.txt");
		fake->Attributes = 0;
		fake->IsFake = true;
		fake->FileSize = nVlen;
		fake->FakeFileContent = _strdup(&val[0]);
		m_pRootDir->AddFile(fake);
	}
	
	return ERROR_SUCCESS;
}

int CMsiViewer::readMediaSources()
{
	UINT res;
	PMSIHANDLE hQueryMedia;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Media ORDER BY `LastSequence`", &hQueryMedia) );
	OK( MsiViewExecute(hQueryMedia, 0) );

	// Retrieve all media entries
	PMSIHANDLE hMediaRec;
	MediaEntry mEntry;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryMedia, &hMediaRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		mEntry.DiskId = MsiRecordGetInteger(hMediaRec, 1);
		mEntry.LastSequence = MsiRecordGetInteger(hMediaRec, 2);
		READ_STR(hMediaRec, 4, mEntry.Cabinet);

		m_vMedias.push_back(mEntry);
		MsiCloseHandle(hMediaRec);
	}
	
	MsiCloseHandle(hQueryMedia);

	return ERROR_SUCCESS;
}

DirectoryNode* CMsiViewer::GetDirectory( const wchar_t* path )
{
	if (!path || !*path)
		return m_pRootDir;

	DirectoryNode* curNode = m_pRootDir;

	wchar_t *dupPath = _wcsdup(path);
	wchar_t *dir;
	wchar_t *context;
	dir = wcstok_s(dupPath, L"\\", &context);
	while (dir != NULL)
	{
		DirectoryNode* child = NULL;
		for (size_t i = 0; i < curNode->SubDirs.size(); i++)
		{
			DirectoryNode *nextNode = curNode->SubDirs[i];
			if (_wcsicmp(dir, nextNode->TargetName) == 0)
				child = nextNode;
		}

		if (child)
			curNode = child;
		else
		{
			curNode = NULL;
			break;
		}
		
		dir = wcstok_s(NULL, L"\\", &context);
	} //while
	
	free(dupPath);
	return curNode;
}

FileNode* CMsiViewer::GetFile( const int fileIndex )
{
	if (fileIndex < 0 || fileIndex >= (int) m_vFlatIndex.size())
		return NULL;

	BasicNode* node = m_vFlatIndex[fileIndex];
	if (!node->IsDir())
		return (FileNode *) node;
		
	return NULL;
}

const wchar_t* CMsiViewer::getFileStorageName( FileNode* file )
{
	// Check flags first to filter uncompressed files
	if ((file->Attributes & msidbFileAttributesNoncompressed) ||
		((file->Attributes & msidbFileAttributesCompressed) == 0 && (m_nSummaryWordCount & 2) == 0))
		return NULL;

	int mediaIndex = -1;
	for (int i = (int) m_vMedias.size() - 1; i >= 0; i--)
	{
		if (file->SequenceMark <= m_vMedias[i].LastSequence)
			mediaIndex = i;
		else
			break;
	}

	return (mediaIndex < 0) ? NULL : m_vMedias[mediaIndex].Cabinet;
}

int CMsiViewer::DumpFileContent( FileNode *file, const wchar_t *destPath, ExtractProcessCallbacks callbacks )
{
	wstring strFilePath(destPath);
	strFilePath.append(file->TargetName);

	int result = SER_ERROR_SYSTEM;

	if (file->IsFake)
	{
		HANDLE hFile = CreateFileW(strFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, file->GetSytemAttributes(), NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwWritten;
			WriteFile(hFile, file->FakeFileContent, file->FileSize, &dwWritten, NULL);
			CloseHandle(hFile);
			
			result = (dwWritten == file->FileSize) ? SER_SUCCESS : SER_ERROR_WRITE;
		}
		else
		{
			result = SER_ERROR_WRITE;
		}
	}
	else
	{
		const wchar_t *cab = getFileStorageName(file);

		if (!cab || !*cab)
		{
			// Copy real file
			wstring strFullSourcePath = getStoragePath() + file->GetSourcePath();
			HANDLE hSourceFile = CreateFileW(strFullSourcePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hSourceFile != INVALID_HANDLE_VALUE)
			{
				HANDLE hDestFile = CreateFileW(strFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, file->GetSytemAttributes(), NULL);
				if (hDestFile != INVALID_HANDLE_VALUE)
				{
					result = SER_SUCCESS;
					
					if (file->GetSize() > 0)
					{
						char *buf = (char *) malloc(FCOPY_BUF_SIZE);
						DWORD dwBytesRead, dwBytesWritten;
						ProgressContext* pctx = (ProgressContext*) callbacks.signalContext;
						__int64 nFileBytesDone = 0;

						BOOL readResult;
						while ((readResult = ReadFile(hSourceFile, buf, FCOPY_BUF_SIZE, &dwBytesRead, NULL)) == TRUE)
						{
							if (dwBytesRead == 0) break;
							if (!WriteFile(hDestFile, buf, dwBytesRead, &dwBytesWritten, NULL) || (dwBytesRead != dwBytesWritten))
							{
								result = SER_ERROR_WRITE;
								break;
							}
							
							nFileBytesDone += dwBytesRead;
							pctx->nProcessedBytes += dwBytesRead;
							pctx->nCurrentFileProgress = (int) ((nFileBytesDone * 100) / file->GetSize());
							callbacks.FileProgress(callbacks.signalContext);
						}
						
						free(buf);
					}
					CloseHandle(hDestFile);
				}
				else
				{
					result = SER_ERROR_WRITE;
				}
				CloseHandle(hSourceFile);
			}
			else
				result = SER_ERROR_READ;
		}
		else
		{
			wstring strCabPath;
			if (cab[0] == '#')
			{
				// Copy from internal stream
				if (cacheInternalStream(cab))
					strCabPath = m_mStreamCache[cab];
			}
			else
			{
				// Copy from external stream
				strCabPath = getStoragePath();
				strCabPath.append(cab);
			}

			if (strCabPath.length() > 0)
			{
				int extr_res = m_pCabControl->ExtractFile(cab, strCabPath.c_str(), file->Key, strFilePath.c_str());
				result = extr_res ? SER_SUCCESS : SER_ERROR_READ;
			}
		}
	}

	return result;
}

std::wstring CMsiViewer::getStoragePath()
{
	wstring strResult(m_strStorageLocation);
	size_t slashPos = strResult.find_last_of('\\');
	if (slashPos != wstring::npos)
		strResult.erase(slashPos + 1);
	else
		strResult.clear();

	return strResult;
}

int CMsiViewer::cacheInternalStream( const wchar_t* streamName )
{
	wstring strCabPath = m_mStreamCache[streamName];
	if (strCabPath.length() > 0)
		return TRUE;
	
	if (m_strStreamCacheLocation.length() == 0)
	{
		wchar_t wszTmpDir[MAX_PATH];
		GetTempPathW(MAX_PATH, wszTmpDir);
		
		m_strStreamCacheLocation.append(wszTmpDir);
		if (m_strStreamCacheLocation.at(m_strStreamCacheLocation.length() - 1) != '\\')
			m_strStreamCacheLocation.append(L"\\");

		FILETIME currentTime;
		wchar_t tmpFolderName[30] = {0};
		GetSystemTimeAsFileTime(&currentTime);
		wsprintfW(tmpFolderName, L"ob%x%x.TMP\\", currentTime.dwHighDateTime, currentTime.dwLowDateTime);
		m_strStreamCacheLocation.append(tmpFolderName);

		if (!CreateDirectoryW(m_strStreamCacheLocation.c_str(), NULL))
		{
			m_strStreamCacheLocation.clear();
			return FALSE;
		}
	}

	int nResult = FALSE;

	UINT res;
	PMSIHANDLE hQueryStream;
	OK( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM _Streams", &hQueryStream) );
	OK( MsiViewExecute(hQueryStream, 0) );

	// Go through streams list and copy to temp folder
	PMSIHANDLE hStreamRec;
	DWORD nCellSize;
	wchar_t wszStreamName[256];
	while ((res = MsiViewFetch(hQueryStream, &hStreamRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hStreamRec, 1, wszStreamName);
		if (wcscmp(wszStreamName, streamName + 1) == 0)
		{
			wstring strCabPath = m_strStreamCacheLocation + (streamName + 1);
			HANDLE hOutputFile = CreateFileW(strCabPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hOutputFile == INVALID_HANDLE_VALUE) break;

			char* buf = (char *) malloc(FCOPY_BUF_SIZE);
			DWORD dwCopySize = FCOPY_BUF_SIZE;
			DWORD dwWritten;
			do 
			{
				res = MsiRecordReadStream(hStreamRec, 2, buf, &dwCopySize);
				if (res != ERROR_SUCCESS) break;

				WriteFile(hOutputFile, buf, dwCopySize, &dwWritten, NULL);
				
			} while (dwCopySize > 0);
			free(buf);

			CloseHandle(hOutputFile);
			m_mStreamCache[streamName] =  strCabPath;
			nResult = (res == ERROR_SUCCESS);
			break;
		}
	}
	MsiCloseHandle(hQueryStream);

	return nResult;
}

void CMsiViewer::buildFlatIndex(DirectoryNode* root)
{
	for (int i = 0, max_i = root->Files.size(); i < max_i; i++)
	{
		m_vFlatIndex.push_back(root->Files[i]);
	}

	for (int i = 0, max_i = root->SubDirs.size(); i < max_i; i++)
	{
		DirectoryNode* subdir = root->SubDirs[i];
		m_vFlatIndex.push_back(subdir);
		buildFlatIndex(subdir);
	}
}

bool CMsiViewer::readRealFileAttributes(FileNode* file)
{
	if (file->IsFake) return false;

	const wchar_t *cab = getFileStorageName(file);
	if (!cab || !*cab)
	{
		// For real files
		wstring strFullSourcePath = getStoragePath() + file->GetSourcePath();
		
		WIN32_FIND_DATAW fd;
		HANDLE hSearch = FindFirstFile(strFullSourcePath.c_str(), &fd);
		if (hSearch != INVALID_HANDLE_VALUE)
		{
			file->ftCreationTime = fd.ftCreationTime;
			file->ftModificationTime = fd.ftLastWriteTime;
			FindClose(hSearch);
			return true;
		}
	}
	else if (cab[0] == L'#')
	{
		// For files in internal cab
	}
	else
	{
		// For files in external cab
		wstring strCabPath = getStoragePath();
		strCabPath.append(cab);

		WIN32_FIND_DATAW fd;
		if (m_pCabControl->GetFileAttributes(cab, strCabPath.c_str(), file->Key, fd))
		{
			file->ftCreationTime = fd.ftCreationTime;
			file->ftModificationTime = fd.ftLastWriteTime;
			
			return true;
		}
	}
	return false;
}

bool CMsiViewer::FindNodeDataByIndex(int itemIndex, LPWIN32_FIND_DATAW dataBuf, wchar_t* itemPathBuf, size_t itemPathBufSize)
{
	if (itemIndex < 0 || itemIndex >= (int) m_vFlatIndex.size())
		return false;

	BasicNode* node = m_vFlatIndex[itemIndex];

	wcscpy_s(dataBuf->cFileName, MAX_PATH, node->TargetName);
	wcscpy_s(dataBuf->cAlternateFileName, MAX_SHORT_NAME_LEN, node->TargetShortName);
	dataBuf->dwFileAttributes = node->GetSytemAttributes();
	dataBuf->nFileSizeLow = (DWORD) node->GetSize();

	// Retrieve file attributes from real files
	if (!node->IsDir())
	{
		if (memcmp(&(node->ftCreationTime), &ZERO_FILE_TIME, sizeof(FILETIME)) == 0)
		{
			FileNode* file = static_cast<FileNode*>(node);
			readRealFileAttributes(file);
		}
		dataBuf->ftCreationTime = node->ftCreationTime;
		dataBuf->ftLastWriteTime = node->ftModificationTime;
	}

	wstring path = node->GetTargetPath();
	wmemcpy_s(itemPathBuf, itemPathBufSize, path.c_str(), path.length());
	itemPathBuf[path.length()] = 0;
	
	return true;
}

int CMsiViewer::GetCompressionType()
{
	if (m_vMedias.size() == 0)
		return MSI_COMPRESSION_NONE;
	if (m_nSummaryWordCount & 2)
		return MSI_COMPRESSION_CAB;

	bool fFoundUncomp = false, fFoundComp = false;
	for (vector<BasicNode*>::const_iterator cit = m_vFlatIndex.begin(); cit != m_vFlatIndex.end(); cit++)
	{
		BasicNode* node = *cit;
		if (!node->IsDir())
		{
			FileNode *fnode = (FileNode *) node;
			if (fnode->IsFake) continue;

			if ((fnode->Attributes & msidbFileAttributesCompressed) != 0)
				fFoundComp = true;
			else
				fFoundUncomp = true;

			if (fFoundComp && fFoundUncomp)
				return MSI_COMPRESSION_MIXED;
		}
	}
	
	return MSI_COMPRESSION_CAB;
}
