#ifndef FarStorage_h__
#define FarStorage_h__

#include "ModulesController.h"
#include "CommonFunc.h"
#include "ContentStructures.h"

class StorageObject
{
private:
	StorageObject() {};

private:
	ModulesController* m_pModules;
	
	int m_nModuleIndex;
	INT_PTR *m_pStoragePtr;
	wchar_t *m_wszStoragePath;

	ContentTreeNode* m_pRootDir;
	ContentTreeNode* m_pCurrentDir;
	vector<ContentTreeNode*> m_vItems;

	__int64 m_nTotalSize;
	int m_nNumFiles;
	int m_nNumDirectories;

public:
	StorageGeneralInfo GeneralInfo;
	
	StorageObject(ModulesController *modules);
	~StorageObject();

	int Open(const wchar_t *Path);
	int ReadFileList();
	void Close();

	int Extract(ExtractOperationParams &params);
	int ChangeCurrentDir(const wchar_t* path);

	ContentTreeNode* GetItem(size_t index);

	ContentTreeNode* CurrentDir() const { return m_pCurrentDir; }
	const wchar_t* GetModuleName() const { return (m_nModuleIndex >= 0) ? m_pModules->modules[m_nModuleIndex].ModuleName : NULL; }
	const wchar_t* StoragePath() const { return m_wszStoragePath; }
	__int64 TotalSize() const { return m_nTotalSize; }
	int NumFiles() const { return m_nNumFiles; }
	int NumDirectories() const { return m_nNumDirectories; }
};

#endif // FarStorage_h__