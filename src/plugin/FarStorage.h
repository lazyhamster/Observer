#ifndef FarStorage_h__
#define FarStorage_h__

#include "ModulesController.h"
#include "ContentStructures.h"

typedef bool(*PasswordQueryCallbackFunc)(char*, size_t);

class StorageObject
{
private:
	StorageObject() {}
	StorageObject(const StorageObject& copy) {}
	StorageObject &operator=(const StorageObject &a) {}

private:
	ModulesController* m_pModules;
	
	int m_nModuleIndex;
	HANDLE m_pStoragePtr;
	wchar_t *m_wszStoragePath;

	ContentTreeNode* m_pRootDir;
	ContentTreeNode* m_pCurrentDir;
	vector<ContentTreeNode*> m_vItems;

	__int64 m_nTotalSize;
	__int64 m_nTotalPackedSize;
	int m_nNumFiles;
	int m_nNumDirectories;

	PasswordQueryCallbackFunc m_fnPassCallback;

public:
	StorageGeneralInfo GeneralInfo;
	
	StorageObject(ModulesController *modules, PasswordQueryCallbackFunc PassCallback);
	~StorageObject();

	int Open(const wchar_t* path, bool applyExtFilters, int openWithModule);
	int ReadFileList(bool &aborted);
	void Close();

	int Extract(ExtractOperationParams &params);
	int ChangeCurrentDir(const wchar_t* path);

	ContentTreeNode* CurrentDir() const { return m_pCurrentDir; }
	const wchar_t* GetModuleName() const { return (m_nModuleIndex >= 0) ? m_pModules->GetModule(m_nModuleIndex).Name() : NULL; }
	const wchar_t* StoragePath() const { return m_wszStoragePath; }
	__int64 TotalSize() const { return m_nTotalSize; }
	__int64 TotalPackedSize() const { return m_nTotalPackedSize; }
	int NumFiles() const { return m_nNumFiles; }
	int NumDirectories() const { return m_nNumDirectories; }
};

#endif // FarStorage_h__