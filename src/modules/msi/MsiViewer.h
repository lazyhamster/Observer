#pragma once

#include "ContentStructs.h"
#include "CabControl.h"
#include "../ModuleDef.h"

typedef map<wstring, DirectoryNode*> DirectoryNodesMap;
typedef map<wstring, ComponentEntry> ComponentEntryMap;
typedef map<wstring, wstring> WStringMap;

class CMsiViewer
{
private:
	MSIHANDLE m_hMsi;
	DirectoryNode* m_pRootDir;
	vector <MediaEntry> m_vMedias;
	wstring m_strStorageLocation;
	
	CCabControl* m_pCabControl;
	wstring m_strStreamCacheLocation;
	map<wstring, wstring> m_mStreamCache;
	
	int readDirectories(DirectoryNodesMap &nodemap);
	int readComponents(DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap);
	int readFiles(DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap);
	int readMediaSources();
	int readAppSearch(WStringMap &entries);
	int readCreateFolder(WStringMap &entries);

	int assignParentDirs(DirectoryNodesMap &nodemap);
	void removeEmptyFolders(DirectoryNode *root, WStringMap &forcedFolders);
	void mergeDotFolders(DirectoryNode *root);
	void avoidSameNames(DirectoryNode *root);
	void checkShortNames(DirectoryNode *root);

	int generateInfoText();
	int generateLicenseText();
	int dumpRegistryKeys(wstringstream &sstr);
	int dumpFeatures(wstringstream &sstr);

	wstring getStoragePath();
	int cacheInternalStream(const wchar_t* streamName);

	int assignSequenceIndicies(DirectoryNode* root, int rootIndex);
	bool FindIndexedNodeData(DirectoryNode* root, int rootIndex, int itemIndex, LPWIN32_FIND_DATAW dataBuf, wchar_t* itemPathBuf, size_t itemPathBufSize);

public:
	CMsiViewer(void);
	~CMsiViewer(void);

	int Open(const wchar_t* path);

	int GetTotalDirectories() { return m_pRootDir->GetSubDirsCount(); }
	int GetTotalFiles() { return m_pRootDir->GetFilesCount(); }
	__int64 GetTotalSize() { return m_pRootDir->GetTotalSize(); }

	DirectoryNode* GetDirectory(const wchar_t* path);
	FileNode* GetFile(const wchar_t* path);

	int DumpFileContent(FileNode *file, const wchar_t *destPath);

	bool FindNodeDataByIndex(int itemIndex, LPWIN32_FIND_DATAW dataBuf, wchar_t* itemPathBuf, size_t itemPathBufSize);
};
