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

	UINT readDirectories(DirectoryNodesMap &nodemap);
	UINT readComponents(ComponentEntryMap &componentmap);
	UINT readFiles(DirectoryNodesMap &nodemap, ComponentEntryMap &componentmap);
	UINT readMediaSources();
	UINT readAppSearch(WStringMap &entries);
	UINT readCreateFolder(WStringMap &entries);
	UINT readEmbeddedFiles(DirectoryNodesMap &nodemap);
	UINT readPackageType();

	void assignParentDirs(DirectoryNodesMap &nodemap, bool processSpecialDirs);
	void removeEmptyFolders(DirectoryNode *root, WStringMap &forcedFolders);
	void mergeDotFolders(DirectoryNode *root);
	void checkShortNames(DirectoryNode *root);
	void mergeSameNamedFolders(DirectoryNode *root);

	UINT iterateOptionalMsiTable(const wchar_t* tableName, std::function<void(MSIHANDLE)> func);

	UINT generateInfoText();
	UINT generateLicenseText();
	UINT dumpRegistryKeys(std::wstringstream &sstr);
	UINT dumpFeatures(std::wstringstream &sstr);
	UINT dumpShortcuts(std::wstringstream &sstr);
	UINT dumpProperties(std::wstringstream &sstr);
	UINT dumpServices(std::wstringstream &sstr);
	UINT dumpEnvironmentVars(std::wstringstream &sstr);
	UINT dumpCustomActions(std::wstringstream &sstr);

	std::wstring getStoragePath();
	bool AcquireStreamCachePath();
	bool cacheInternalStream(const wchar_t* streamName);

	void buildFlatIndex(DirectoryNode* root);

	const wchar_t* getFileStorageName(FileNode* file);
	bool readRealFileAttributes(FileNode* file);
	FileNode* getFileByComponentName(const DirectoryNode* dir, const std::wstring& compName);

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
