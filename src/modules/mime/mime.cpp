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

static bool optCheckMandatoryHeaders = true;

struct MimeFileInfo
{
	GMimeParser* parserRef;
	GMimeMessage* messageRef;

	std::vector<GMimeObject*> messageParts;
	GMimeStream* headersDecoded;
	FILETIME msgTime;

	MimeFileInfo() : parserRef(NULL), messageRef(NULL), msgTime(), headersDecoded(NULL) {}
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

static void headers_foreach_callback(const char *name, const char *value, gpointer user_data)
{
	GMimeStream* memStrm = (GMimeStream*) user_data;
	char* szDecodedValue = g_mime_utils_header_decode_text(value);

	g_mime_stream_write_string(memStrm, name);
	g_mime_stream_write_string(memStrm, ": ");
	g_mime_stream_write_string(memStrm, szDecodedValue);
	g_mime_stream_write_string(memStrm, "\r\n");

	g_free(szDecodedValue);
}

static void generate_headers_fake_file(MimeFileInfo *info)
{
	GMimeStream* memStrm = g_mime_stream_mem_new();
	g_mime_stream_mem_set_owner((GMimeStreamMem*) memStrm, TRUE);

	GMimeHeaderList* headers = g_mime_object_get_header_list((GMimeObject*) info->messageRef);
	g_mime_header_list_foreach(headers, headers_foreach_callback, memStrm);

	info->headersDecoded = memStrm;
	//TODO: implement
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

	if (optCheckMandatoryHeaders)
	{
		// File should have at least one of the checked headers to be considered a MIME file
		const char* hdr_ctype_str = g_mime_object_get_header((GMimeObject*) message, "Content-Type");
		const char* hdr_from_str = g_mime_object_get_header((GMimeObject*) message, "From");
		if (!hdr_ctype_str && !hdr_from_str)
		{
			g_object_unref(parser);
			g_object_unref(message);
			return SOR_INVALID_FILE;
		}
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
	generate_headers_fake_file(minfo);
	
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
		g_object_unref(minfo->headersDecoded);

		delete minfo;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	MimeFileInfo *minfo = (MimeFileInfo*) storage;
	if (minfo == NULL) return GET_ITEM_ERROR;

	if (item_index < 0) return GET_ITEM_ERROR;
	if (item_index > (int)minfo->messageParts.size()) return GET_ITEM_NOMOREITEMS;

	// Index 0 is a fake file with decoded headers

	GMimeObject* pObj = item_index > 0 ? minfo->messageParts[item_index-1] : NULL;

	memset(item_info, 0, sizeof(StorageItemInfo));
	item_info->Attributes = FILE_ATTRIBUTE_NORMAL;
	item_info->ModificationTime = minfo->msgTime;

	if (item_index == 0)
	{
		// Headers file
		wcscat_s(item_info->Path, STRBUF_SIZE(item_info->Path), L"{headers}");
		item_info->Size = 0;
		if (minfo->headersDecoded != NULL)
		{
			GByteArray* bytePtr = g_mime_stream_mem_get_byte_array((GMimeStreamMem*) minfo->headersDecoded);
			item_info->Size = bytePtr->len;
		}
	}
	else if (GMIME_IS_MESSAGE_PART (pObj))
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

	if ( (params.ItemIndex < 0) || (params.ItemIndex > (int) minfo->messageParts.size()) )
		return SER_ERROR_SYSTEM;

	FILE* dfh;
	if (_wfopen_s(&dfh, params.DestPath, L"wb") != 0)
		return SER_ERROR_WRITE;

	GMimeObject* pObj = params.ItemIndex > 0 ? minfo->messageParts[params.ItemIndex-1] : nullptr;

	int retVal = SER_ERROR_READ;
	if (params.ItemIndex == 0)
	{
		GMimeStream* destStream = g_mime_stream_file_new(dfh);
		g_mime_stream_file_set_owner((GMimeStreamFile*) destStream, FALSE);

		g_mime_stream_seek(minfo->headersDecoded, 0, GMIME_STREAM_SEEK_SET);
		g_mime_stream_write_to_stream(minfo->headersDecoded, destStream);
		g_object_unref(destStream);
		retVal = SER_SUCCESS;
	}
	else if (GMIME_IS_MESSAGE_PART (pObj))
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
		DeleteFile(params.DestPath);

	return retVal;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	g_mime_init(0);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
