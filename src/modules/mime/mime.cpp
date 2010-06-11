// mime.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include <vector>

#include "mimetic/mimetic.h"

using namespace mimetic;
using namespace std;

struct MimeFileInfo
{
	wchar_t *path;
	MimeEntity *entity;
	vector<MimeEntity*> children;
};

wstring GetEntityName(MimeEntity* entity)
{
	static int i = 0;
	
	wchar_t buf[100] = {0};
	swprintf_s(buf, 100, L"file%d.bin", i++);
	return buf;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	char szInput[MAX_PATH] = {0};
	WideCharToMultiByte(CP_ACP, 0, path, wcslen(path), szInput, MAX_PATH, NULL, NULL);
	
	File filePtr(szInput);
	if (!filePtr) return FALSE;

	MimeEntity *me = new MimeEntity();
	me->load(filePtr.begin(), filePtr.end());
	if (me->body().parts().size() > 0)
	{
		MimeFileInfo *minfo = new MimeFileInfo();
		minfo->path = _wcsdup(path);
		minfo->entity = me;

		*storage = (INT_PTR*) minfo;

		Header &head = me->header();
		MimeVersion &mimeVer = head.mimeVersion();
		ContentType &ctype = head.contentType();

		memset(info, 0, sizeof(StorageGeneralInfo));
		swprintf_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MIME %d.%d", mimeVer.maj(), mimeVer.minor());
		swprintf_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"%S/%S", ctype.type().c_str(), ctype.subtype().c_str());
		wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"Mixed");

		// Cache child list
		MimeEntityList& parts = me->body().parts();
		MimeEntityList::iterator mbit = parts.begin(), meit = parts.end();
		for(; mbit != meit; ++mbit)
		{
			MimeEntity *childEn = *mbit;
			minfo->children.push_back(childEn);
		}

		return TRUE;
	}
	
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo)
	{
		minfo->children.clear();
		free(minfo->path);
		delete minfo->entity;
		delete minfo;
	}
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	if (item_index < 0) return GET_ITEM_ERROR;

	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo == NULL) return GET_ITEM_ERROR;

	if (item_index >= (int) minfo->children.size())
		return GET_ITEM_NOMOREITEMS;
	
	MimeEntity* entiry = minfo->children[item_index];
	if (!entiry) return GET_ITEM_ERROR;

	wstring name = GetEntityName(entiry);
	
	wcscpy_s(item_path, path_size, name.c_str());

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, name.c_str());
	wcscpy_s(item_data->cAlternateFileName, 14, L"");
	item_data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	item_data->nFileSizeLow = entiry->size();
	//item_data->ftLastWriteTime = ;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
}
