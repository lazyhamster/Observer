// pdf.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include "PdfInfo.h"
#include "poppler/GlobalParams.h"

static bool optOpenAllFiles = false;

#define FILE_SIGNATURE_PDF "%PDF-"

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	if (!SignatureMatchOrNull(params.Data, params.DataSize, FILE_SIGNATURE_PDF))
		return SOR_INVALID_FILE;
	
	PDFDoc* doc = new PDFDoc(params.FilePath, (int) wcslen(params.FilePath));
	if (doc->isOk())
	{
		PdfInfo* pData = new PdfInfo();
		if (pData->LoadInfo(doc, optOpenAllFiles))
		{
			*storage = pData;

			memset(info, 0, sizeof(StorageGeneralInfo));
			swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"PDF %d.%d Document", doc->getPDFMajorVersion(), doc->getPDFMinorVersion());
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

			return SOR_SUCCESS;
		}
		delete pData;
	}
	delete doc;
	
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData != nullptr)
	{
		pData->Cleanup();
		delete pData;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData == nullptr || item_index < 0) return GET_ITEM_ERROR;

	if (item_index < pData->GetNumFiles())
	{
		const PdfPseudoFile* pFile = pData->GetFile(item_index);

		memset(item_info, 0, sizeof(StorageItemInfo));
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"%s%s", pFile->GetPrefix(), pFile->GetName());
		item_info->Size = pFile->GetSize();
		item_info->PackedSize = pFile->GetSize();
		pFile->GetCreationTime(&item_info->CreationTime);
		pFile->GetModificationTime(&item_info->ModificationTime);

		RenameInvalidPathChars(item_info->Path + wcslen(pFile->GetPrefix()));
		
		// For some special cases
		if (item_info->Size < 0) item_info->Size = 0;
		if (item_info->PackedSize < 0) item_info->PackedSize = 0;

		return GET_ITEM_OK;
	}

	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData == nullptr || params.ItemIndex < 0 || params.ItemIndex >= pData->GetNumFiles()) return SER_ERROR_SYSTEM;

	const PdfPseudoFile* pFile = pData->GetFile(params.ItemIndex);
	PseudoFileSaveResult sr = pFile->Save(params.DestPath);

	switch (sr)
	{
	case PseudoFileSaveResult::PFSR_OK:
		return SER_SUCCESS;
	case PseudoFileSaveResult::PFSR_ERROR_READ:
		return SER_ERROR_READ;
	case PseudoFileSaveResult::PFSR_ERROR_WRITE:
		return SER_ERROR_WRITE;
	}
	
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {993197B5-FD1B-49A6-B87F-18599F43D0AA}
static const GUID MODULE_GUID = { 0x993197b5, 0xfd1b, 0x49a6, { 0xb8, 0x7f, 0x18, 0x59, 0x9f, 0x43, 0xd0, 0xaa } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 1);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	globalParams = new GlobalParams();
	globalParams->setErrQuiet(gTrue);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
	delete globalParams;
}
