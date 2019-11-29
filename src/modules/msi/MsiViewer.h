#pragma once

#include "ContentStructs.h"
#include "CabControl.h"
#include "ModuleDef.h"

typedef std::map<std::wstring, DirectoryNode*> DirectoryNodesMap;
typedef std::map<std::wstring, ComponentEntry> ComponentEntryMap;
typedef std::map<std::wstring, std::wstring> WStringMap;

#define MSI_OPENFLAG_KEEPUNIQUEDIRS 1
#define MSI_OPENFLAG_SHOWSPECIALS 2

#define MSI_COMPRESSION_NONE 0
#define MSI_COMPRESSION_CAB 1
#define MSI_COMPRESSION_MIXED 2

enum class MsiFileType
{
	Unknown,
	MSI,
	MergeModule
};

class CMsiViewer
{
private:
	MSIHANDLE m_hMsi;
	DirectoryNode* m_pRootDir;
	std::vector <MediaEntry> m_vMedias;
	std::wstring m_strStorageLocation;
	std::vector <BasicNode*> m_vFlatIndex;
	int m_nSummaryWordCount;
	FILETIME m_ftCreateTime;
	MsiFileType m_eType;
	
	CCabControl* m_pCabControl;
	std::wstring m_strStreamCacheLocation;
	std::map<std::wstring, std::wstring> m_mStreamCache;

	int readDirectories(DirectoryNodesMap &nodemap);
	int readComponents(ComponentEntryMap &componentmap);
	int readFiles(DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap);
	int readMediaSources();
	int readAppSearch(WStringMap &entries);
	int readCreateFolder(WStringMap &entries);
	int readEmbeddedFiles(DirectoryNodesMap &nodemap);
	int readPackageType();

	int assignParentDirs(DirectoryNodesMap &nodemap, bool processSpecialDirs);
	void removeEmptyFolders(DirectoryNode *root, WStringMap &forcedFolders);
	void mergeDotFolders(DirectoryNode *root);
	void checkShortNames(DirectoryNode *root);
	void mergeSameNamedFolders(DirectoryNode *root);

	int generateInfoText();
	int generateLicenseText();
	int dumpRegistryKeys(wstringstream &sstr);
	int dumpFeatures(wstringstream &sstr);
	int dumpShortcuts(wstringstream &sstr);
	int dumpProperties(wstringstream &sstr);
	int dumpServices(wstringstream &sstr);

	wstring getStoragePath();
	bool AcquireStreamCachePath();
	bool cacheInternalStream(const wchar_t* streamName);

	void buildFlatIndex(DirectoryNode* root);

	const wchar_t* getFileStorageName(FileNode* file);
	bool readRealFileAttributes(FileNode* file);

public:
	CMsiViewer(void);
	~CMsiViewer(void);

	int Open(const wchar_t* path, DWORD openFlags);

	int GetTotalDirectories() { return m_pRootDir->GetSubDirsCount(); }
	int GetTotalFiles() { return m_pRootDir->GetFilesCount(); }
	__int64 GetTotalSize() { return m_pRootDir->GetTotalSize(); }
	int GetCompressionType();
	FILETIME GetCreateDateTime() { return m_ftCreateTime; };
	MsiFileType GetFileType() { return m_eType; }

	DirectoryNode* GetDirectory(const wchar_t* path);
	FileNode* GetFile(const int fileIndex);

	int DumpFileContent(FileNode *file, const wchar_t *destFilePath, ExtractProcessCallbacks callbacks);

	bool FindNodeDataByIndex(int itemIndex, StorageItemInfo* item_info);
};
