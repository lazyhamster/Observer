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
	if (folder == NULL) return;
	
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
	HLPackageType ePackageType = GetPackageType(params.FilePath);
	if (ePackageType == HL_PACKAGE_NONE)
		return SOR_INVALID_FILE;
	
	CPackage* pkg = CreatePackage(ePackageType);
	if (pkg->Open(params.FilePath, HL_MODE_READ) && pkg->GetRoot())
	{
		ValvePackage* vp = new ValvePackage();
		vp->ePackageType = HL_PACKAGE_GCF;
		vp->pPackage = pkg;

		EnumFiles(vp, pkg->GetRoot());

		*storage = vp;
		
		memset(info, 0, sizeof(StorageGeneralInfo));
		MultiByteToWideChar(CP_ACP, 0, pkg->GetDescription(), -1, info->Format, STORAGE_FORMAT_NAME_MAX_LEN);
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

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	ValvePackage* vp = (ValvePackage*) storage;
	if (vp == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= (int) vp->vItems.size())
		return GET_ITEM_NOMOREITEMS;

	CDirectoryFile* fileItem = vp->vItems[item_index];

	memset(item_info, 0, sizeof(StorageItemInfo));
	item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
	item_info->Size = fileItem->GetSize();
	item_info->PackedSize = fileItem->GetSizeOnDisk();

	hlChar path[1024] = {0};
	fileItem->GetPath(path, sizeof(path));
	MultiByteToWideChar(CP_ACP, 0, path, -1, item_info->Path, STRBUF_SIZE(item_info->Path));

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	ValvePackage* vp = (ValvePackage*) storage;
	if (vp == NULL) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= (int) vp->vItems.size())
		return SER_ERROR_SYSTEM;

	CDirectoryFile* fileItem = vp->vItems[params.item];
	if (fileItem->Extract(params.destFilePath) == hlTrue)
		return SER_SUCCESS;
	
	return SER_ERROR_READ;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
