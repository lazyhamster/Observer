// udfimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "UdfIn.h"

struct CRef2
{
	int Vol;
	int Fs;
	int Ref;
};

struct UdfStorage
{
	CUdfArchive arc;
	CRecordVector<CRef2> refs2;
};

void CopyArcParam(wchar_t* dest, UString src)
{
	size_t maxSize = STORAGE_PARAM_MAX_LEN;
	
	if (src.IsEmpty())
		wcscpy_s(dest, maxSize, L"-");
	else if ((size_t) src.Length() <= maxSize)
		wcscpy_s(dest, maxSize + 1, src);
	else
		wcscpy_s(dest, maxSize + 1, src.Left((int) maxSize));
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	UdfStorage *storageRec = new UdfStorage;

	if (storageRec->arc.Open(params.FilePath, NULL))
	{
		*storage = storageRec;

		// Create reference links array
		char udfVerMajor = 0, udfVerMinor = 0;
		bool showVolName = (storageRec->arc.LogVols.Size() > 1);
		for (int volIndex = 0; volIndex < storageRec->arc.LogVols.Size(); volIndex++)
		{
			const CLogVol &vol = storageRec->arc.LogVols[volIndex];
			bool showFileSetName = (vol.FileSets.Size() > 1);
			for (int fsIndex = 0; fsIndex < vol.FileSets.Size(); fsIndex++)
			{
				const CFileSet &fs = vol.FileSets[fsIndex];
				for (int i = ((showVolName || showFileSetName) ? 0 : 1); i < fs.Refs.Size(); i++)
				{
					CRef2 ref2;
					ref2.Vol = volIndex;
					ref2.Fs = fsIndex;
					ref2.Ref = i;
					storageRec->refs2.Add(ref2);
				}
			}
			udfVerMajor = vol.DomainId.Suffix[1];
			udfVerMinor = vol.DomainId.Suffix[0];
		} //for volIndex

		if (udfVerMajor > 0)
			swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"UDF %x.%02x", udfVerMajor, udfVerMinor);
		else
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"UDF");
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");
		CopyArcParam(info->Comment, storageRec->arc.GetComment());
		info->Created = storageRec->arc.GetCreatedTime();

		return SOR_SUCCESS;
	}
	
	delete storageRec;
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	UdfStorage *storageRec = (UdfStorage*) storage;
	if (storageRec)
		delete storageRec;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	UdfStorage *storageRec = (UdfStorage*) storage;
	if (!storageRec) return GET_ITEM_ERROR;

	if ((item_index < 0) || (item_index >= storageRec->refs2.Size()))
		return GET_ITEM_NOMOREITEMS;

	const CRef2 &ref2 = storageRec->refs2[item_index];
	const CLogVol &vol = storageRec->arc.LogVols[ref2.Vol];
	const CRef &ref = vol.FileSets[ref2.Fs].Refs[ref2.Ref];
	const CFile &file = storageRec->arc.Files[ref.FileIndex];
	CItem &item = storageRec->arc.Items[file.ItemIndex];

	UString fname = file.GetName();

	memset(item_info, 0, sizeof(StorageItemInfo));
	if (item.IsDir())
		item_info->Attributes = FILE_ATTRIBUTE_DIRECTORY;
	else
		item_info->Attributes = file.IsHidden ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_ARCHIVE;
	item_info->Size = item.Size;
	item.MTime.GetFileTime(item_info->ModificationTime);
	item.CreateTime.GetFileTime(item_info->CreationTime);

	UString fullPath = storageRec->arc.GetItemPath(ref2.Vol, ref2.Fs, ref2.Ref, storageRec->arc.LogVols.Size() > 1, vol.FileSets.Size() > 1);
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), fullPath);
	
	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	UdfStorage *storageRec = (UdfStorage*) storage;
	if (!storageRec || (params.item < 0)) return SER_ERROR_SYSTEM;

	const CRef2 &ref2 = storageRec->refs2[params.item];
	const CLogVol &vol = storageRec->arc.LogVols[ref2.Vol];
	const CRef &ref = vol.FileSets[ref2.Fs].Refs[ref2.Ref];
	const CFile &file = storageRec->arc.Files[ref.FileIndex];
	CItem &item = storageRec->arc.Items[file.ItemIndex];

	if (!item.IsDir())
	{
		int res = storageRec->arc.DumpFileContent(file.ItemIndex, ref.FileIndex, params.destFilePath, &(params.callbacks));

		return res;
	}
	
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
