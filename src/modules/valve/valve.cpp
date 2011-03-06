// valve.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "HLLib/Packages.h"
#include "HLLib/Streams.h"
#include "AuxUtils.h"

using namespace HLLib;
using namespace HLLib::Streams;

struct ValvePackage
{
	CPackage* pPackage;
	HLPackageType ePackageType;
	std::vector<CDirectoryFile*> vItems;
};

void EnumFiles(ValvePackage* package, CDirectoryFolder* folder)
{
	for (hlUInt i = 0; i < folder->GetCount(); i++)
	{
		CDirectoryItem* child = folder->GetItem(i);
		switch (child->GetType())
		{
			case HL_ITEM_FOLDER:
				EnumFiles(package, (CDirectoryFolder*) child);
				break;
			case HL_ITEM_FILE:
				package->vItems.push_back((CDirectoryFile*) child);
				break;
		}
	}
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	hlChar* lpFilename = "G:\\9\\valve\\half-life engine.gcf";
	wchar_t* lpwFilename = L"G:\\9\\valve\\half-life engine.gcf";

	HLPackageType ePackageType = GetPackageType(lpwFilename);
	if (ePackageType == HL_PACKAGE_NONE)
		return SOR_INVALID_FILE;
	
	CPackage* pkg = CreatePackage(ePackageType);
	if (pkg->Open(lpFilename, HL_MODE_READ))
	{
		if (ePackageType == HL_PACKAGE_NCF)
		{
			static_cast<CNCFFile *>(pkg)->SetRootPath(NULL);
		}
		
		ValvePackage* vp = new ValvePackage();
		vp->ePackageType = HL_PACKAGE_GCF;
		vp->pPackage = pkg;

		EnumFiles(vp, pkg->GetRoot());

		*storage = vp;
		
		memset(info, 0, sizeof(StorageGeneralInfo));
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, pkg->GetDescription());
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Mixed");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
		
		return SOR_SUCCESS;
	}

	delete pkg;
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	ValvePackage* vp = (ValvePackage*) storage;
	if (vp != NULL)
	{
		vp->pPackage->Close();
		delete vp->pPackage;
		delete vp;
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	ValvePackage* vp = (ValvePackage*) storage;
	if (vp == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= (int) vp->vItems.size())
		return GET_ITEM_NOMOREITEMS;

	CDirectoryFile* fileItem = vp->vItems[item_index];

	//wcscpy_s(item_path, path_size, L"");
	
	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	//wcscpy_s(item_data->cFileName, MAX_PATH, L"");
	item_data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	item_data->nFileSizeLow = fileItem->GetSize();

	hlChar path[1024] = {0};
	fileItem->GetPath(path, sizeof(path));
	MultiByteToWideChar(CP_ACP, 0, path, -1, item_path, path_size);
	MultiByteToWideChar(CP_ACP, 0, fileItem->GetName(), -1, item_data->cFileName, MAX_PATH);

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	ValvePackage* vp = (ValvePackage*) storage;
	if (vp == NULL) return SER_ERROR_SYSTEM;
	
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->OpenStorage = OpenStorage;
	LoadParams->CloseStorage = CloseStorage;
	LoadParams->GetItem = GetStorageItem;
	LoadParams->ExtractItem = ExtractItem;

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
