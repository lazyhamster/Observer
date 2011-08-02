// mime.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "ModuleCRT.h"

#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>

#include "NameDecode.h"

struct MimeFileInfo
{
	GMimeParser* parserRef;
	GMimeMessage* messageRef;

	std::vector<GMimeObject*> messageParts;
	FILETIME msgTime;

	MimeFileInfo() : parserRef(NULL), messageRef(NULL), msgTime() {}
};

static bool is_mbox(int fh)
{
	long pos = _tell(fh);
	
	char buf[10] = {0};
	int numBytes = _read(fh, buf, sizeof(buf));
	bool fResult = (numBytes > 4) && (strncmp(buf, "From ", 5) == 0);

	_lseek(fh, pos, SEEK_SET);
	return fResult;
}

static void message_foreach_callback(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
	MimeFileInfo* mi = (MimeFileInfo*) user_data;
	
	if (GMIME_IS_MULTIPART (part))
		; // Skip these parts
	else
		mi->messageParts.push_back(part);
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	__int64 nSize = GetFileSize(params.FilePath);
	if (nSize < 10 || nSize > 100 * 1024 * 1024)
		return SOR_INVALID_FILE;
	
	int fh;
	if (_wsopen_s(&fh, params.FilePath, _O_RDONLY, _SH_DENYWR, _S_IREAD) != 0)
		return SOR_INVALID_FILE;

	// Check if we are dealing with mbox file
	if (is_mbox(fh))
	{
		_close(fh);
		return SOR_INVALID_FILE;
	}
	
	GMimeStream* stream = g_mime_stream_fs_new(fh);
	g_mime_stream_fs_set_owner((GMimeStreamFs*)stream, TRUE);

	GMimeParser* parser = g_mime_parser_new_with_stream(stream);
	g_mime_parser_set_persist_stream(parser, TRUE);
	g_mime_parser_set_scan_from(parser, FALSE);
	g_object_unref(stream);

	GMimeMessage* message = g_mime_parser_construct_message(parser);
	if (message == NULL)
	{
		g_object_unref(parser);
		return SOR_INVALID_FILE;
	}

	GMimeObject* mime_part = g_mime_message_get_mime_part(message);
	GMimeContentType* ctype = g_mime_object_get_content_type(mime_part);
	const char* szType = ctype ? g_mime_content_type_get_media_type(ctype) : NULL;
	const char* szSubType = ctype ? g_mime_content_type_get_media_subtype(ctype) : NULL;

	time_t tMsgTime = 0;
	int nTimeZone = 0;
	g_mime_message_get_date(message, &tMsgTime, &nTimeZone);

	MimeFileInfo* minfo = new MimeFileInfo();
	minfo->messageRef = message;
	minfo->parserRef = parser;
	if (tMsgTime) UnixTimeToFileTime(tMsgTime, &minfo->msgTime);

	g_mime_message_foreach(message, message_foreach_callback, minfo);
	
	*storage = minfo;

	memset(info, 0, sizeof(StorageGeneralInfo));
	wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"MIME");
	swprintf_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"%S/%S", szType ? szType : "-" , szSubType ? szSubType : "-");
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo)
	{
		g_object_unref(minfo->messageRef);
		g_object_unref(minfo->parserRef);

		delete minfo;
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo == NULL) return GET_ITEM_ERROR;

	if (item_index < 0) return GET_ITEM_ERROR;
	if (item_index >= (int)minfo->messageParts.size()) return GET_ITEM_NOMOREITEMS;

	GMimeObject* pObj = minfo->messageParts[item_index];

	memset(item_info, 0, sizeof(StorageItemInfo));
	item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
	item_info->ModificationTime = minfo->msgTime;

	if (GMIME_IS_MESSAGE_PART (pObj))
	{
		/* message/rfc822 or message/news */
		GMimeMessage* subMsg = g_mime_message_part_get_message((GMimeMessagePart*) pObj);
		const char* subj = g_mime_message_get_subject(subMsg);
		if (subj)
		{
			MultiByteToWideChar(CP_UTF8, 0, subj, -1, item_info->Path, STRBUF_SIZE(item_info->Path));
			wcscat_s(item_info->Path, STRBUF_SIZE(item_info->Path), L".eml");
		}
		else
		{
			swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"%04d.eml", item_index);
		}

		GMimeStream* nullStream = g_mime_stream_null_new();
		item_info->Size = g_mime_object_write_to_stream((GMimeObject*) subMsg, nullStream);
		g_object_unref(nullStream);
	}
	else if (GMIME_IS_MESSAGE_PARTIAL (pObj))
	{
		/* message/partial */
		/* this is an incomplete message part, probably a large message that the sender has broken into smaller parts and is sending us bit by bit. */
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"%04d - partial", item_index);
	}
	else if (GMIME_IS_MULTIPART (pObj))
	{
		/* multipart/*. This branch should not be reached in normal circumstances. */
		swprintf_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"%04d - multi part", item_index);
	}
	else if (GMIME_IS_PART (pObj))
	{
		/* a normal leaf part, could be text/plain or image/jpeg etc */
		GetEntityName((GMimePart*) pObj, item_info->Path, STRBUF_SIZE(item_info->Path));
		
		GMimeStream* nullStream = g_mime_stream_null_new();
		GMimeDataWrapper* wrap = g_mime_part_get_content_object((GMimePart*) pObj);
		item_info->Size = wrap ? g_mime_data_wrapper_write_to_stream(wrap, nullStream) : 0;
		
		g_object_unref(nullStream);
	}
	else
	{
		// Should not reach this
		return GET_ITEM_ERROR;
	}

	RenameInvalidPathChars(item_info->Path);
	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo == NULL) return SER_ERROR_SYSTEM;

	if ( (params.item < 0) || (params.item >= (int) minfo->messageParts.size()) )
		return SER_ERROR_SYSTEM;

	FILE* dfh;
	if (_wfopen_s(&dfh, params.destFilePath, L"wb") != 0)
		return SER_ERROR_WRITE;

	GMimeObject* pObj = minfo->messageParts[params.item];

	int retVal = SER_ERROR_READ;
	if (GMIME_IS_MESSAGE_PART (pObj))
	{
		GMimeMessage* subMsg = g_mime_message_part_get_message((GMimeMessagePart*) pObj);

		GMimeStream* destStream = g_mime_stream_file_new(dfh);
		g_mime_stream_file_set_owner((GMimeStreamFile*) destStream, FALSE);

		g_mime_object_write_to_stream((GMimeObject*) subMsg, destStream);
		g_object_unref(destStream);
		retVal = SER_SUCCESS;
	}
	else if (GMIME_IS_MESSAGE_PARTIAL (pObj))
	{
		//
	}
	else if (GMIME_IS_MULTIPART (pObj))
	{
		// Should not have this case
	}
	else if (GMIME_IS_PART (pObj))
	{
		GMimeDataWrapper* wrap = g_mime_part_get_content_object((GMimePart*) pObj);
		
		if (wrap)
		{
			GMimeStream* destStream = g_mime_stream_file_new(dfh);
			g_mime_stream_file_set_owner((GMimeStreamFile*) destStream, FALSE);

			g_mime_data_wrapper_write_to_stream(wrap, destStream);
		
			g_object_unref(destStream);
			retVal = SER_SUCCESS;
		}
	}

	fclose(dfh);
	if (retVal != SER_SUCCESS)
		DeleteFile(params.destFilePath);

	return retVal;
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
