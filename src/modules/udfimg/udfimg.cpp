// udfimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\ModuleDef.h"
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

int MODULE_EXPORT LoadSubModule(int Reserved)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	UdfStorage *storageRec = new UdfStorage;

	if (storageRec->arc.Open(path, NULL))
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

		DWORD nTotalFiles = 0, nTotalDirs = 0;
		__int64 nTotalSize = 0;
		for (int i = 1; i < storageRec->arc.Items.Size(); i++) // skip root dir on index 0
		{
			CItem &item = storageRec->arc.Items[i];
			if (item.IsDir())
				nTotalDirs++;
			else
				nTotalFiles++;
			nTotalSize += item.Size;
		}

		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"UDF");
		info->NumFiles = nTotalFiles;
		info->NumDirectories = nTotalDirs;
		info->TotalSize = nTotalSize;
		info->NumRealItems = nTotalFiles + nTotalDirs;

		return TRUE;
	}
	
	delete storageRec;
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	UdfStorage *storageRec = (UdfStorage*) storage;
	if (storageRec)
		delete storageRec;
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	UdfStorage *storageRec = (UdfStorage*) storage;
	if (!storageRec) return FALSE;

	if ((item_index < 0) || (item_index > storageRec->refs2.Size()))
		return FALSE;

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

	UString fullPath = storageRec->arc.GetItemPath(ref2.Vol, ref2.Fs, ref2.Ref, storageRec->arc.LogVols.Size() > 1, vol.FileSets.Size() > 1);
	wcscpy_s(item_path, path_size, fullPath);
	
	return TRUE;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	UdfStorage *storageRec = (UdfStorage*) storage;
	if (!storageRec) return SER_ERROR_SYSTEM;

	int itemIndex = storageRec->arc.GetItemByPath(params.item);
	if (itemIndex < 0) return SER_ERROR_SYSTEM;

	if (!storageRec->arc.Items[itemIndex].IsDir())
	{
		int res = storageRec->arc.DumpFileContent(itemIndex, params.destPath, &(params.Callbacks));
		return res;
	}
	
	return SER_ERROR_SYSTEM;
}
