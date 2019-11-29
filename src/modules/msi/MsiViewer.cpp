#include "StdAfx.h"
#include "MsiViewer.h"
#include "OleReader.h"

#include <MsiQuery.h>
#include <MsiDefs.h>

#pragma comment(lib, "Msi.lib")

#define OK_V(f, rv) res=f; \
	if(res!=ERROR_SUCCESS) return rv;
#define OK(f) res=f; \
	if(res!=ERROR_SUCCESS) return res;
#define OK_MISS(f) res=f; \
	if(res == ERROR_BAD_QUERY_SYNTAX) return ERROR_SUCCESS; \
	else if(res != ERROR_SUCCESS) return res;
#define READ_STR(rec,ind,s) nCellSize = sizeof(s)/sizeof(s[0]); OK( MsiRecordGetStringW(rec, ind, s, &nCellSize) );
#define READ_STR_V(rec,ind,s, rv) nCellSize = sizeof(s)/sizeof(s[0]); OK_V( MsiRecordGetStringW(rec, ind, s, &nCellSize), rv );

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
	{L"LocalAppDataFolder", L"LocalA~1"},		//The full path to the folder that contains local (non-roaming) applications. 
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
	m_eType = MsiFileType::Unknown;
}

CMsiViewer::~CMsiViewer(void)
{
	delete m_pCabControl;
	if (m_hMsi)	MsiCloseHandle(m_hMsi);
	if (m_pRootDir)	delete m_pRootDir;

	for (auto iter = m_mStreamCache.begin(); iter != m_mStreamCache.end(); iter++)
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
	OK ( readComponents(mComponents) );

	// Get File entry list
	OK ( readFiles(mDirs, mComponents) );

	// List all embedded binary streams as files
	OK ( readEmbeddedFiles(mDirs) );

	// Check type of package
	OK ( readPackageType() );

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
	mergeSameNamedFolders(m_pRootDir);

	mCreateFolder.clear();
	mComponents.clear();
	mDirs.clear();

	// Generate fake files content
	OK ( generateInfoText() );
	OK ( generateLicenseText() );

	buildFlatIndex(m_pRootDir);

	m_strStorageLocation = path;
	m_pCabControl->SetOwner(m_hMsi);
	
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
	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Directory", &hQueryDirs) );
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

		auto citer = appSearch.find(dirEntry.Key);
		bool fIsAppSearch = (citer != appSearch.end());

		DirectoryNode *node = new DirectoryNode();
		if (node->Init(&dirEntry, fIsAppSearch))
			nodemap[dirEntry.Key] = node;
		else
			delete node;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::readComponents( ComponentEntryMap &componentmap )
{
	UINT res;
	PMSIHANDLE hQueryComp;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Component", &hQueryComp) );
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

	// Lets skip files query if no components or directories are found
	if (nodemap.size() == 0 || componentmap.size() == 0)
		return ERROR_SUCCESS;

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

		// Sometimes there are strange files with empty component, let's just skip them
		if (!fileEntry.Component[0] || !wcscmp(fileEntry.Component, L" "))
			continue;

		// If file refers to non-existing component then skip it
		if (componentmap.find(fileEntry.Component) == componentmap.end())
			continue;

		FileNode *node = new FileNode();
		node->Init(&fileEntry);

		const ComponentEntry &component = componentmap[fileEntry.Component];
		DirectoryNode *dir = nodemap[component.Directory];
		
		if (!dir)
		{
			dir = new DirectoryNode();
			dir->Init(component.Directory);
			nodemap[component.Directory] = dir;
		}
		dir->AddFile(node);
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

int CMsiViewer::readEmbeddedFiles( DirectoryNodesMap &nodemap )
{
	UINT res;
	PMSIHANDLE hQueryBinaries;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Binary", &hQueryBinaries) );
	OK( MsiViewExecute(hQueryBinaries, 0) );

	// Add base dir for fake embedded files
	DirectoryEntry dirEntry = {0};
	wcscpy_s(dirEntry.Key, sizeof(dirEntry.Key) / sizeof(dirEntry.Key[0]), L"$embedded-v07g1k$");
	wcscpy_s(dirEntry.DefaultName, sizeof(dirEntry.DefaultName) / sizeof(dirEntry.DefaultName[0]), L"{embedded}");
	
	DirectoryNode *dirNode = new DirectoryNode();
	if (dirNode->Init(&dirEntry, false))
		nodemap[dirEntry.Key] = dirNode;

	wchar_t name[128];

	// Retrieve all component entries
	PMSIHANDLE hBinRec;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryBinaries, &hBinRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hBinRec, 1, name);
		
		// Cache entire stream content in memory
		DWORD nStreamSize = MsiRecordDataSize(hBinRec, 2);
		char* pStreamData = (char*) malloc(nStreamSize);
		MsiRecordReadStream(hBinRec, 2, pStreamData, &nStreamSize);

		FileNode *embFile = new FileNode();
		embFile->TargetName = _wcsdup(name);
		embFile->TargetShortName = _wcsdup(L"");
		embFile->Attributes = 0;
		embFile->IsFake = true;
		embFile->FileSize = nStreamSize;
		embFile->FakeFileContent = pStreamData;
		dirNode->AddFile(embFile);
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::readPackageType()
{
	UINT res;
	PMSIHANDLE hQuery;

	// Merge Module must have ModuleComponents table with some data
	// If this table is absent of empty then we have regular MSI database.

	m_eType = MsiFileType::MSI;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM _Streams", &hQuery) );
	OK( MsiViewExecute(hQuery, 0) );

	PMSIHANDLE hCompRec;
	wchar_t wszStreamName[256] = {0};
	DWORD nCellSize;

	while ((res = MsiViewFetch(hQuery, &hCompRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hCompRec, 1, wszStreamName);
		if (wcscmp(wszStreamName, L"MergeModule.CABinet") == 0)
		{
			m_eType = MsiFileType::MergeModule;
			break;
		}
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::assignParentDirs( DirectoryNodesMap &nodemap, bool processSpecialDirs )
{
	int numSpecFolders = processSpecialDirs ? sizeof(MsiSpecialFolders) / sizeof(MsiSpecialFolders[0]) : 0;
	
	for (auto dirIter = nodemap.begin(); dirIter != nodemap.end(); ++dirIter)
	{
		DirectoryNode* node = dirIter->second;
		DirectoryNode* parent = (node->ParentKey) ? nodemap[node->ParentKey] : m_pRootDir;

		// This should not occur under normal circumstances but happens sometimes
		if (!parent && (parent != m_pRootDir))
		{
			parent = new DirectoryNode();
			parent->Init(node->ParentKey);
			m_pRootDir->AddSubdir(parent);

			nodemap[node->ParentKey] = parent;
		}

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
	
	for (auto iter = root->SubDirs.begin(); iter != root->SubDirs.end();)
	{
		DirectoryNode* subDir = *iter;
		removeEmptyFolders(subDir, forcedFolders);

		auto citer = forcedFolders.find(subDir->Key);
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

void CMsiViewer::checkShortNames(DirectoryNode *root)
{
	for (auto iter = root->SubDirs.begin(); iter != root->SubDirs.end();)
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
	} //for

	// Second pass to merge everything inside sub-folders again
	for (size_t i = 0; i < nMaxCounter; i++)
	{
		DirectoryNode* subdir = root->SubDirs[i];
		mergeSameNamedFolders(subdir);
	}
}

int CMsiViewer::generateInfoText()
{
	struct PropDescription
	{
		wchar_t* PropName;
		UINT PropID;
	};
	PropDescription SUMMARY_PROPS[] = {
		{L"Title", PID_TITLE}, {L"Subject", PID_SUBJECT}, {L"Author", PID_AUTHOR}, {L"Codepage", PID_CODEPAGE},
		{L"Keywords", PID_KEYWORDS}, {L"Comments", PID_COMMENTS}, {L"Template", PID_TEMPLATE}, {L"Revision Number", PID_REVNUMBER}
	};
	
	wstringstream sstr;
	UINT res;

	sstr << L"Installation package general info\n\n";

	// Summary info
	sstr << endl;
	sstr << L"[Summary Info]" << endl;
	
	PMSIHANDLE hSummary;
	OK ( MsiGetSummaryInformationW(m_hMsi, NULL, 0, &hSummary) );
	
	UINT nDataType;
	INT nIntVal;
	DWORD len;
	vector<wchar_t> propdata(512);
	for (int i = 0; i < sizeof(SUMMARY_PROPS) / sizeof(SUMMARY_PROPS[0]); i++)
	{
		while ((res = MsiSummaryInfoGetPropertyW(hSummary, SUMMARY_PROPS[i].PropID, &nDataType, &nIntVal, NULL, &propdata[0], &(len = (DWORD) propdata.size()))) == ERROR_MORE_DATA)
			propdata.resize(len + 1);

		if (res == ERROR_SUCCESS)
		{
			if ((nDataType == 30 /*VT_LPSTR*/) || (nDataType == 31 /*VT_LPWSTR*/))
				sstr << SUMMARY_PROPS[i].PropName << L": " << (wchar_t *) &propdata[0] << endl;
			else if ((nDataType == 2 /*VT_I2*/) || (nDataType == 3 /*VT_I4*/))
				sstr << SUMMARY_PROPS[i].PropName << L": " << nIntVal << endl;
		}
	}

	// Read "Word Count" property to save default compression info
	int nPropVal;
	if (MsiSummaryInfoGetPropertyW(hSummary, PID_WORDCOUNT, &nDataType, &nPropVal, NULL, NULL, NULL) == ERROR_SUCCESS)
		m_nSummaryWordCount = nPropVal;
	
	// Save "Create Time/Date" property for future use
	FILETIME ftPropVal;
	if (MsiSummaryInfoGetPropertyW(hSummary, PID_CREATE_DTM, &nDataType, NULL, &ftPropVal, NULL, NULL) == ERROR_SUCCESS)
		m_ftCreateTime = ftPropVal;

	// Content info
	sstr << endl;
	sstr << L"Total directories: " << GetTotalDirectories() << endl;
	sstr << L"Total files: " << GetTotalFiles() << endl;

	// Features list
	sstr << endl;
	sstr << L"[Available Features]" << endl;
	OK( dumpFeatures(sstr) );

	// Registry keys
	sstr << endl;
	sstr << L"[Registry]" << endl;
	OK ( dumpRegistryKeys(sstr) );

	// Shortcuts list
	sstr << endl;
	sstr << L"[Shortcuts]" << endl;
	OK ( dumpShortcuts(sstr) );

	// Services list
	sstr << endl;
	sstr << L"[Installable Services]" << endl;
	OK ( dumpServices(sstr) );

	// Properties
	sstr << endl;
	sstr << L"[Properties]" << endl;
	OK ( dumpProperties(sstr) );

	wstring content = sstr.str();

	// Read "Last Save Time/Date" property
	FILETIME ftSaveTimeVal = {0};
	MsiSummaryInfoGetPropertyW(hSummary, PID_LASTSAVE_DTM, &nDataType, NULL, &ftSaveTimeVal, NULL, NULL);
	
	// Add fake file with general info to root folder
	FileNode *fake = new FileNode();
	fake->TargetName = _wcsdup(L"{msi_info}.txt");
	fake->TargetShortName = _wcsdup(L"msi_info.txt");
	fake->Attributes = FILE_ATTRIBUTE_NORMAL;
	fake->IsFake = true;
	fake->FileSize = (INT32) (content.size() * sizeof(wchar_t) + 2);	// sizeof BOM = 2
	fake->FakeFileContent = (char *) malloc(fake->FileSize + sizeof(wchar_t)); // +1 for 0-terminator
	fake->ftCreationTime = m_ftCreateTime;
	fake->ftModificationTime = ftSaveTimeVal;
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
	wchar_t wszPrevRegKeyName[512] = {0};
	UINT16 nPrevRegRoot = UINT16(-1);
	while ((res = MsiViewFetch(hQueryReg, &hRegRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hRegRec, 1, regEntry.Key);
		regEntry.Root = MsiRecordGetInteger(hRegRec, 2);
		READ_STR(hRegRec, 3, regEntry.RegKeyName);
		READ_STR(hRegRec, 4, regEntry.Name);

		while ((res = MsiRecordGetStringW(hRegRec, 5, &regval[0], &(nVlen = (DWORD)regval.size()))) == ERROR_MORE_DATA)
			regval.resize(nVlen + 1);
		OK (res);

		// Print registry key name (if not equal to previous one)
		if (nPrevRegRoot != regEntry.Root || wcscmp(wszPrevRegKeyName, regEntry.RegKeyName) != 0)
		{
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

			nPrevRegRoot = regEntry.Root;
			wcscpy_s(wszPrevRegKeyName, sizeof(wszPrevRegKeyName) / sizeof(wszPrevRegKeyName[0]), regEntry.RegKeyName);
		}

		// Print registry key value
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

int CMsiViewer::dumpShortcuts(wstringstream &sstr)
{
	UINT res;
	PMSIHANDLE hQueryShortcut;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Shortcut", &hQueryShortcut) );
	OK( MsiViewExecute(hQueryShortcut, 0) );

	// Retrieve all feature entries
	PMSIHANDLE hShortcutRec;
	DWORD nCellSize;
	ShortcutEntry sEntry;
	while ((res = MsiViewFetch(hQueryShortcut, &hShortcutRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hShortcutRec, 1, sEntry.Key);
		READ_STR(hShortcutRec, 2, sEntry.Directory);
		READ_STR(hShortcutRec, 3, sEntry.Name);
		READ_STR(hShortcutRec, 4, sEntry.Component);
		READ_STR(hShortcutRec, 5, sEntry.Target);
		READ_STR(hShortcutRec, 6, sEntry.Arguments);
		READ_STR(hShortcutRec, 7, sEntry.Description);

		wchar_t *wszNamePtr = wcschr(sEntry.Name, '|');
		wszNamePtr = (wszNamePtr != NULL) ? wszNamePtr + 1 : sEntry.Name;

		sstr << L"- " << sEntry.Key << L" (" << wszNamePtr << L")\n";
		sstr << sEntry.Directory << L" -> " << sEntry.Target;
		if (wcslen(sEntry.Arguments) > 0)
			sstr << L" " << sEntry.Arguments;
		sstr << L"\n";
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::dumpProperties(wstringstream &sstr)
{
	UINT res;
	PMSIHANDLE hQueryProps;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM Property", &hQueryProps) );
	OK( MsiViewExecute(hQueryProps, 0) );

	wchar_t wszPropertyName[512];
	vector<wchar_t> vPropertyData(1024);

	// Retrieve all feature entries
	PMSIHANDLE hPropRec;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQueryProps, &hPropRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hPropRec, 1, wszPropertyName);
		
		while (MsiRecordGetString(hPropRec, 2, &vPropertyData[0], &(nCellSize = (DWORD) vPropertyData.size())) == ERROR_MORE_DATA)
			vPropertyData.resize(nCellSize + 1);

		if (wszPropertyName[0])
			sstr << wszPropertyName << L" = " << &vPropertyData[0] << endl;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::dumpServices(wstringstream &sstr)
{
	UINT res;
	PMSIHANDLE hQuerySvc;

	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM ServiceInstall", &hQuerySvc) );
	OK( MsiViewExecute(hQuerySvc, 0) );

	ServiceInstallEntry siEntry;

	// Retrieve all feature entries
	PMSIHANDLE hSvcRec;
	DWORD nCellSize;
	while ((res = MsiViewFetch(hQuerySvc, &hSvcRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		READ_STR(hSvcRec, 2, siEntry.Name);
		READ_STR(hSvcRec, 3, siEntry.DisplayName);
		siEntry.ServiceType = MsiRecordGetInteger(hSvcRec, 4);
		siEntry.StartType = MsiRecordGetInteger(hSvcRec, 5);
		READ_STR(hSvcRec, 9, siEntry.StartName);
		READ_STR(hSvcRec, 10, siEntry.Password);
		READ_STR(hSvcRec, 11, siEntry.Arguments);
		READ_STR(hSvcRec, 12, siEntry.Component);
		READ_STR(hSvcRec, 13, siEntry.Description);

		sstr << siEntry.Name << endl;
		if (siEntry.DisplayName[0]) sstr << L"\tDisplay Name: " << siEntry.DisplayName << endl;
		sstr << L"\tDescription: " << siEntry.Description << endl;
		sstr << L"\tCommand: " << siEntry.Component << L" " << siEntry.Arguments << endl;

		sstr << L"\tService Type:";
		if (siEntry.ServiceType & SERVICE_WIN32_OWN_PROCESS) sstr << L" SERVICE_WIN32_OWN_PROCESS";
		if (siEntry.ServiceType & SERVICE_WIN32_SHARE_PROCESS) sstr << L" SERVICE_WIN32_SHARE_PROCESS";
		if (siEntry.ServiceType & SERVICE_INTERACTIVE_PROCESS) sstr << L" SERVICE_INTERACTIVE_PROCESS";
		if (siEntry.ServiceType & SERVICE_KERNEL_DRIVER) sstr << L" SERVICE_KERNEL_DRIVER";
		if (siEntry.ServiceType & SERVICE_FILE_SYSTEM_DRIVER) sstr << L" SERVICE_FILE_SYSTEM_DRIVER";
		sstr << endl;

		sstr << L"\tStart Type:";
		if (siEntry.StartType & SERVICE_AUTO_START) sstr << L" Automatic";
		else if (siEntry.StartType & SERVICE_DEMAND_START) sstr << L" Manual";
		else if (siEntry.StartType & SERVICE_DISABLED) sstr << L" Disabled";
		else if (siEntry.StartType & SERVICE_BOOT_START) sstr << L" Boot Start";
		else if (siEntry.StartType & SERVICE_SYSTEM_START) sstr << L" System Start";
		else sstr << L" Unknown";
		sstr << endl;

		sstr << L"\tAccount: " << (siEntry.StartName[0] ? siEntry.StartName : L"[LocalSystem]") << endl;
		if (siEntry.Password[0]) sstr << L"\tPassword: " << siEntry.Password << endl;
	}

	return ERROR_SUCCESS;
}

int CMsiViewer::generateLicenseText()
{
	UINT res;
	PMSIHANDLE hQueryLicense;
	
	OK_MISS( MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM `Control` WHERE (`Control`.`Control` = 'AgreementText') OR (`Control`.`Control` = 'LicenseText')", &hQueryLicense) );
	OK( MsiViewExecute(hQueryLicense, 0) );

	// Retrieve all registry entries
	PMSIHANDLE hLicRec;
	DWORD nVlen = 0;
	vector<char> val(512);
	while ((res = MsiViewFetch(hQueryLicense, &hLicRec)) != ERROR_NO_MORE_ITEMS)
	{
		OK(res);

		while ((res = MsiRecordGetStringA(hLicRec, 10, &val[0], &(nVlen = (DWORD)val.size()))) == ERROR_MORE_DATA)
			val.resize(nVlen + 1);
		
		MsiCloseHandle(hLicRec);
		OK (res); 
	}

	MsiCloseHandle(hQueryLicense);

	if (nVlen > 0)
	{
		const char* cntText = &val[0];

		// Add fake file with general info to root folder
		FileNode *fake = new FileNode();
		if (_strnicmp(cntText, "{\\rtf", 4) == 0)
		{
			fake->TargetName = _wcsdup(L"{license}.rtf");
			fake->TargetShortName = _wcsdup(L"license.rtf");
		}
		else
		{
			fake->TargetName = _wcsdup(L"{license}.txt");
			fake->TargetShortName = _wcsdup(L"license.txt");
		}
		fake->Attributes = 0;
		fake->IsFake = true;
		fake->FileSize = nVlen;
		fake->FakeFileContent = _strdup(cntText);
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
	if (m_eType == MsiFileType::MergeModule)
		return L"MergeModule.CABinet";
	
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

int CMsiViewer::DumpFileContent( FileNode *file, const wchar_t *destFilePath, ExtractProcessCallbacks callbacks )
{
	int result = SER_ERROR_SYSTEM;

	if (file->IsFake)
	{
		HANDLE hFile = CreateFileW(destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, file->GetSytemAttributes(), NULL);
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
				HANDLE hDestFile = CreateFileW(destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, file->GetSytemAttributes(), NULL);
				if (hDestFile != INVALID_HANDLE_VALUE)
				{
					result = SER_SUCCESS;
					
					if (file->GetSize() > 0)
					{
						char *buf = (char *) malloc(FCOPY_BUF_SIZE);
						DWORD dwBytesRead, dwBytesWritten;

						BOOL readResult;
						while ((readResult = ReadFile(hSourceFile, buf, FCOPY_BUF_SIZE, &dwBytesRead, NULL)) != FALSE)
						{
							if (dwBytesRead == 0) break;
							if (!WriteFile(hDestFile, buf, dwBytesRead, &dwBytesWritten, NULL) || (dwBytesRead != dwBytesWritten))
							{
								result = SER_ERROR_WRITE;
								break;
							}
							
							if (!callbacks.FileProgress(callbacks.signalContext, dwBytesRead))
							{
								result = SER_USERABORT;
								break;
							}
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
			if (cab[0] == '#' || m_eType == MsiFileType::MergeModule)
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
				bool extr_res = m_pCabControl->ExtractFile(cab, strCabPath.c_str(), file->Key, destFilePath);
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

bool CMsiViewer::AcquireStreamCachePath()
{
	if (m_strStreamCacheLocation.length() > 0)
		return true;

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

	if (!CreateDirectory(m_strStreamCacheLocation.c_str(), NULL))
	{
		m_strStreamCacheLocation.clear();
		return false;
	}

	return true;
}

bool CMsiViewer::cacheInternalStream( const wchar_t* streamName )
{
	wstring strCabPath = m_mStreamCache[streamName];
	if (strCabPath.length() > 0)
		return true;
	
	if (!AcquireStreamCachePath())
		return false;

	const wchar_t* msiInternalStreamName = streamName[0] == '#' ? streamName + 1 : streamName;
	strCabPath = m_strStreamCacheLocation + msiInternalStreamName;

	UINT res;
	PMSIHANDLE hQueryStream;
	
	res = MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM _Streams", &hQueryStream);
	if (res == ERROR_INVALID_HANDLE)
	{
		// Very rare bug with auto-closed handle, try to reopen it
		res = MsiOpenDatabaseW(m_strStorageLocation.c_str(), MSIDBOPEN_READONLY, &m_hMsi);
		if (res == ERROR_SUCCESS)
			res = MsiDatabaseOpenViewW(m_hMsi, L"SELECT * FROM _Streams", &hQueryStream);
	}
	if (res == ERROR_BAD_QUERY_SYNTAX)
	{
		// Could be a WiX file. In this case we should read it as Compound File.
		COleStorage oleStor;
		if (!oleStor.Open(m_strStorageLocation.c_str()))
			return false;
		if (oleStor.ExtractStream(msiInternalStreamName, strCabPath.c_str()))
		{
			m_mStreamCache[streamName] = strCabPath;
			return true;
		}
	}
	OK_V(res, false);

	OK_V(MsiViewExecute(hQueryStream, 0), false);

	bool nResult = false;

	// Go through streams list and copy to temp folder
	PMSIHANDLE hStreamRec;
	DWORD nCellSize;
	wchar_t wszStreamName[256];
	while ((res = MsiViewFetch(hQueryStream, &hStreamRec)) != ERROR_NO_MORE_ITEMS)
	{
		if (res != ERROR_SUCCESS)
			break;

		READ_STR_V(hStreamRec, 1, wszStreamName, false);
		if (wcscmp(wszStreamName, msiInternalStreamName) == 0)
		{
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

	return nResult;
}

void CMsiViewer::buildFlatIndex(DirectoryNode* root)
{
	for (size_t i = 0, max_i = root->Files.size(); i < max_i; i++)
	{
		m_vFlatIndex.push_back(root->Files[i]);
	}

	for (size_t i = 0, max_i = root->SubDirs.size(); i < max_i; i++)
	{
		DirectoryNode* subdir = root->SubDirs[i];
		m_vFlatIndex.push_back(subdir);
		buildFlatIndex(subdir);
	}
}

bool CMsiViewer::readRealFileAttributes(FileNode* file)
{
	if (!file || file->IsFake) return false;

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
		WIN32_FIND_DATAW fd;
		if (m_pCabControl->GetFileAttributes(cab, NULL, file->Key, fd))
		{
			file->ftCreationTime = fd.ftCreationTime;
			file->ftModificationTime = fd.ftLastWriteTime;
			if (file->GetSize() == 0 && fd.nFileSizeLow != 0)
				file->FileSize = fd.nFileSizeLow;

			return true;
		}
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

bool CMsiViewer::FindNodeDataByIndex(int itemIndex, StorageItemInfo* item_info)
{
	if (itemIndex < 0 || itemIndex >= (int) m_vFlatIndex.size())
		return false;

	BasicNode* node = m_vFlatIndex[itemIndex];

	// Retrieve file attributes from real files
	if (!node->IsDir())
	{
		if (memcmp(&(node->ftCreationTime), &ZERO_FILE_TIME, sizeof(FILETIME)) == 0)
		{
			FileNode* file = dynamic_cast<FileNode*>(node);
			readRealFileAttributes(file);
		}
		item_info->CreationTime = node->ftCreationTime;
		item_info->ModificationTime = node->ftModificationTime;
	}

	item_info->Size = node->GetSize();
	item_info->Attributes = node->GetSytemAttributes();

	wstring path = node->GetTargetPath();
	wmemcpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), path.c_str(), path.length());
	item_info->Path[path.length()] = 0;
	
	return true;
}

int CMsiViewer::GetCompressionType()
{
	if (m_vMedias.size() == 0)
		return MSI_COMPRESSION_NONE;
	if (m_nSummaryWordCount & 2)
		return MSI_COMPRESSION_CAB;

	bool fFoundUncomp = false, fFoundComp = false;
	for (auto cit = m_vFlatIndex.cbegin(); cit != m_vFlatIndex.cend(); ++cit)
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
