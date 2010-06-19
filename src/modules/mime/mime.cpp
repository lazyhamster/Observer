// mime.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

#include <vector>
#include <fstream>

#include "mimetic/mimetic.h"
#include "mimetic/rfc822/datetime.h"
#include "NameDecode.h"

using namespace mimetic;
using namespace std;

struct MimeFileInfo
{
	wchar_t *path;
	MimeEntity *entity;
	vector<MimeEntity*> children;
	vector<wstring> childNames;
};

static FILETIME ConvertDateTime(DateTime &dt)
{
	SYSTEMTIME stime = {0};
	stime.wYear = dt.year();
	stime.wMonth = dt.month().ordinal();
	stime.wDay = dt.day();
	stime.wHour = dt.hour();
	stime.wMinute = dt.minute();
	stime.wSecond = dt.second();

	FILETIME res = {0};
	SystemTimeToFileTime(&stime, &res);

	// Apply time-zone shift to convert to UTC
	int nTimeZoneShift = dt.zone().ordinal() / 100;
	if (nTimeZoneShift != 0)
	{
		__int64* li = (__int64*)&res;
		*li -= nTimeZoneShift * (__int64)10000000*60*60;
	}

	return res;
}

static void CacheFilesList(MimeEntity* entity, MimeFileInfo* info)
{
	MimeEntityList& parts = entity->body().parts();
	MimeEntityList::iterator mbit = parts.begin(), meit = parts.end();
	for(; mbit != meit; ++mbit)
	{
		MimeEntity *childEn = *mbit;
		ContentType &ctype = childEn->header().contentType();
		if (_stricmp(ctype.type().c_str(), "multipart") == 0)
		{
			CacheFilesList(childEn, info);
		}
		else
		{
			info->children.push_back(childEn);

			wstring strChildName = GetEntityName(childEn);
			info->childNames.push_back(strChildName);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	File filePtr(path);
	if (!filePtr) return FALSE;

	MimeEntity *me = new MimeEntity();
	me->load(filePtr.begin(), filePtr.end());
	if (me->body().parts().size() <= 0)
	{
		delete me;
		return FALSE;
	}

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

	// Read creation date if present
	Field& fdDate = head.field("Date");
	if (fdDate.value().length() > 0)
	{
		DateTime dt(fdDate.value());
		info->Created = ConvertDateTime(dt);
	}

	// Cache child list
	CacheFilesList(me, minfo);

	// Resolve same name problem
	for (int i = 0; i < (int) minfo->childNames.size(); i++)
	{
		int cnt = 1;
		wstring strVal = minfo->childNames[i];
		for (int j = i+1; j < (int) minfo->childNames.size(); j++)
		{
			wstring strNext = minfo->childNames[j];
			if (_wcsicmp(strVal.c_str(), strNext.c_str()) == 0)
			{
				cnt++;
				AppendDigit(strNext, cnt);
				minfo->childNames[j] = strNext;
			}
		}

		if (cnt > 1)
		{
			AppendDigit(strVal, 1);
			minfo->childNames[i] = strVal;
		}
	}

	return TRUE;
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
	
	MimeEntity* entity = minfo->children[item_index];
	if (!entity) return GET_ITEM_ERROR;
	wstring name = minfo->childNames[item_index];
	
	wcscpy_s(item_path, path_size, name.c_str());

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, name.c_str());
	wcscpy_s(item_data->cAlternateFileName, 14, L"");
	item_data->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	item_data->nFileSizeLow = entity->size();

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo == NULL) return SER_ERROR_SYSTEM;

	if ( (params.item < 0) || (params.item >= (int) minfo->children.size()) )
		return SER_ERROR_SYSTEM;

	MimeEntity* entity = minfo->children[params.item];
	if (!entity) return GET_ITEM_ERROR;
	wstring itemName = minfo->childNames[params.item];

	wstring fullPath(params.dest_path);
	fullPath.append(itemName);

	Body& body = entity->body();
	ContentTransferEncoding& enc = entity->header().contentTransferEncoding();
	fstream fs(fullPath.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
	if (fs.is_open())
	{
		string encName = enc.str();
		if (_stricmp(encName.c_str(), "base64") == 0)
		{
			Base64::Decoder dec;
			ostream_iterator<char> oit(fs);
			decode(body.begin(), body.end(), dec, oit);
		}
		else if (_stricmp(encName.c_str(), "quoted-printable") == 0)
		{
			QP::Decoder dec;
			ostream_iterator<char> oit(fs);
			decode(body.begin(), body.end(), dec, oit);
		}
		else
		{
			fs << body;
		}

		fs.close();
		return SER_SUCCESS;
	}
	
	return SER_ERROR_SYSTEM;
}
