// nsis.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\ModuleDef.h"
#include "NsisArchive.h"

#if defined( _7ZIP_LARGE_PAGES)
extern "C"
{
#include "../C/Alloc.h"
}
#endif

#include "7zip/UI/Common/LoadCodecs.h"

int MODULE_EXPORT LoadSubModule(int Reserved)
{
#if defined(_7ZIP_LARGE_PAGES)
	SetLargePageSize();
#endif

	CCodecs *codecs = new CCodecs;
	CMyComPtr<
		#ifdef EXTERNAL_CODECS
		ICompressCodecsInfo
		#else
		IUnknown
		#endif
	> compressCodecsInfo = codecs;
	HRESULT result = codecs->Load();
	if ((result != S_OK) || (codecs->Formats.Size() == 0))
	{
		delete codecs;
		return FALSE;
	}
	
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	CNsisArchive* arc = new CNsisArchive();
	if (!arc->Open(path))
	{
		delete arc;
		return FALSE;
	}

	*storage = (INT_PTR *) arc;
	
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"NSIS");
	wcscpy_s(info->SubType, STORAGE_SUBTYPE_NAME_MAX_LEN, arc->GetSubType());
	info->NumRealItems = arc->GetTotalFiles();

	return TRUE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	CNsisArchive* arc = (CNsisArchive *) storage;
	if (arc)
		delete arc;
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	CNsisArchive* arc = (CNsisArchive *) storage;
	if (!arc) return FALSE;

	int nRes = arc->GetItem(item_index, item_data, item_path, path_size);
	return nRes;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	CNsisArchive* arc = (CNsisArchive *) storage;
	if (!arc) return FALSE;
	
	return arc->ExtractItemByName(params.item, params.destPath, &(params.Callbacks));
}
