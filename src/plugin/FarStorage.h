#ifndef FarStorage_h__
#define FarStorage_h__

#include "ModulesController.h"
#include "ContentStructures.h"

typedef bool(*PasswordQueryCallbackFunc)(char*, size_t);

enum class ListReadResult
{
	Ok,
	Aborted,
	PrepFailed,
	ItemError
};

class StorageObject
{
private:
	StorageObject() = delete;
	StorageObject(const StorageObject& copy) = delete;
	StorageObject &operator=(const StorageObject &a) = delete;

private:
	ModulesController* m_pModules;
	
	int m_nModuleIndex;
	HANDLE m_pStoragePtr;
	wchar_t *m_wszStoragePath;

	ContentTreeNode* m_pRootDir;
	ContentTreeNode* m_pCurrentDir;
	std::vector<ContentTreeNode*> m_vItems;

	__int64 m_nTotalSize;
	__int64 m_nTotalPackedSize;
	int m_nNumFiles;
	int m_nNumDirectories;

	PasswordQueryCallbackFunc m_fnPassCallback;

public:
	StorageGeneralInfo GeneralInfo;
	
	StorageObject(ModulesController *modules, PasswordQueryCallbackFunc PassCallback);
	~StorageObject();

	bool Open(const wchar_t* path, bool applyExtFilters, int openWithModule);
	bool Open(const wchar_t* path, const void* data, size_t dataSize, bool applyExtFilters, int openWithModule);
	ListReadResult ReadFileList();
	void Close();

	int Extract(ExtractOperationParams &params);
	bool ChangeCurrentDir(const wchar_t* path);

	ContentTreeNode* CurrentDir() const { return m_pCurrentDir; }
	const wchar_t* GetModuleName() const { return (m_nModuleIndex >= 0) ? m_pModules->GetModule(m_nModuleIndex)->Name() : NULL; }
	int GetModuleIndex() const { return m_nModuleIndex; }
	const wchar_t* StoragePath() const { return m_wszStoragePath; }
	__int64 TotalSize() const { return m_nTotalSize; }
	__int64 TotalPackedSize() const { return m_nTotalPackedSize; }
	int NumFiles() const { return m_nNumFiles; }
	int NumDirectories() const { return m_nNumDirectories; }
};

#endif // FarStorage_h__