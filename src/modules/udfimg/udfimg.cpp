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

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	UdfStorage *storageRec = new UdfStorage;

	if (storageRec->arc.Open(params.FilePath, NULL))
	{
		*storage = (INT_PTR *) storageRec;

		// Create reference links array
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
		} //for volIndex

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

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
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

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, fname);
	wcscpy_s(item_data->cAlternateFileName, 14, L"");
	item_data->dwFileAttributes = item.IsDir() ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	item_data->nFileSizeLow = (DWORD) item.Size;
	item_data->nFileSizeHigh = (DWORD) (item.Size >> 32);
	item.MTime.GetFileTime(item_data->ftLastWriteTime);
	item.CreateTime.GetFileTime(item_data->ftCreationTime);
	item.ATime.GetFileTime(item_data->ftLastAccessTime);

	UString fullPath = storageRec->arc.GetItemPath(ref2.Vol, ref2.Fs, ref2.Ref, storageRec->arc.LogVols.Size() > 1, vol.FileSets.Size() > 1);
	wcscpy_s(item_path, path_size, fullPath);
	
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
