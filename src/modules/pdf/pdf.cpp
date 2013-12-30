// pdf.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include "PdfInfo.h"
#include "poppler/GlobalParams.h"

static bool optOpenAllFiles = false;

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	PDFDoc* doc = new PDFDoc(params.FilePath, (int) wcslen(params.FilePath));
	if (doc->isOk())
	{
		PdfInfo* pData = new PdfInfo();
		if (pData->LoadInfo(doc) && (optOpenAllFiles || (pData->embFiles.size() > 0)))
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

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData == nullptr || item_index < 0) return GET_ITEM_ERROR;

	memset(item_info, 0, sizeof(StorageItemInfo));
	if (item_index == 0)
	{
		// First file will be always fake info file
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{info}.txt");
		item_info->Size = pData->metadata.size();
		return GET_ITEM_OK;
	}
	else if (item_index < (int) pData->embFiles.size() + 1)
	{
		FileSpec* fsp = pData->embFiles[item_index - 1];
		GooString* strName = fsp->getFileName();
		EmbFile* embFileInfo = fsp->getEmbeddedFile();
		
		wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{files}\\");
		if (strName->hasUnicodeMarker())
		{
			wcscat_s(item_info->Path, STRBUF_SIZE(item_info->Path), (wchar_t*)(strName->getCString() + 2));
		}
		else
		{
			size_t currLen = wcslen(item_info->Path);
			MultiByteToWideChar(CP_ACP, 0, strName->getCString(), strName->getLength(), item_info->Path + currLen, (int) (STRBUF_SIZE(item_info->Path) - currLen));
			item_info->Path[currLen + strName->getLength()] = 0;
		}
		item_info->Size = embFileInfo->size();
		TryParseDateTime(embFileInfo->createDate(), &item_info->CreationTime);
		TryParseDateTime(embFileInfo->modDate(), &item_info->ModificationTime);

		// For some special cases
		if (item_info->Size < 0) item_info->Size = 0;

		return GET_ITEM_OK;
	}
	
	return GET_ITEM_NOMOREITEMS;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	PdfInfo* pData = (PdfInfo*) storage;
	if (pData == nullptr || params.item < 0) return SER_ERROR_SYSTEM;

	if (params.item == 0)
	{
		DWORD dwBytes;
		HANDLE hf = CreateFileW(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		WriteFile(hf, pData->metadata.c_str(), (DWORD) pData->metadata.size(), &dwBytes, NULL);
		CloseHandle(hf);

		return SER_SUCCESS;
	}
	else if (params.item < (int) pData->embFiles.size() + 1)
	{
		FileSpec* fsp = pData->embFiles[params.item - 1];
		EmbFile* embFileInfo = fsp->getEmbeddedFile();

		if (!embFileInfo->isOk())
			return SER_ERROR_READ;

		GBool ret = embFileInfo->save(params.destFilePath);
		return ret ? SER_SUCCESS : SER_ERROR_WRITE;
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

	globalParams = new GlobalParams();

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
	delete globalParams;
}
