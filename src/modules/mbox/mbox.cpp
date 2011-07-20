// mbox.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include "MboxReader.h"
#include "BatReader.h"

static IMailReader* TryContainer(const wchar_t* path)
{
	CMboxReader* mbr = new CMboxReader();
	if (mbr->Open(path))
		return mbr;
	else
		delete mbr;

	CBatReader* btr = new CBatReader();
	if (btr->Open(path))
		return btr;
	else
		delete btr;
	
	return NULL;
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	IMailReader* reader = TryContainer(params.FilePath);
	if (reader == NULL)
		return SOR_INVALID_FILE;

	*storage = reader;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, reader->GetFormatName());
	wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");
	
	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	IMailReader* reader = (IMailReader*) storage;
	if (reader)	delete reader;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	IMailReader* reader = (IMailReader*) storage;
	if (!reader) return GET_ITEM_ERROR;

	if (reader->GetItemsCount() == 0)
		reader->Scan();
	
	if (item_index >= reader->GetItemsCount())
		return GET_ITEM_NOMOREITEMS;

	const MBoxItem &mbi = reader->GetItem(item_index);

	memset(item_info, 0, sizeof(StorageItemInfo));
	if (mbi.Subject.length() > 0)
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"%04d - %.240s.eml", item_index, mbi.Subject.c_str());
	else
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"%04d.eml", item_index);
	RenameInvalidPathChars(item_info->Path);
	item_info->Attributes = mbi.IsDeleted ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
	item_info->Size = mbi.EndPos - mbi.StartPos;
	UnixTimeToFileTime(mbi.Date, &item_info->ModificationTime);

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	IMailReader* reader = (IMailReader*) storage;
	if (!reader) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= reader->GetItemsCount())
		return SER_ERROR_SYSTEM;

	return reader->Extract(params.item, params.destFilePath);
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

	g_mime_init(0);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
