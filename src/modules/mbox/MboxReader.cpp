#include "stdafx.h"
#include "MboxReader.h"

int CMboxReader::Scan()
{
	if (!m_pSrcFile) return -1;
	
	GMimeStream* stream = g_mime_stream_file_new(m_pSrcFile);
	g_mime_stream_file_set_owner((GMimeStreamFile*)stream, FALSE);
	
	GMimeParser* parser = g_mime_parser_new_with_stream(stream);
	g_mime_parser_set_persist_stream(parser, TRUE);
	g_mime_parser_set_scan_from(parser, TRUE);

	GMimeMessage* message;
	gint64 msgStart;
	int nFoundItems = 0;

	m_vItems.clear();
	while (!g_mime_parser_eos(parser))
	{
		msgStart = g_mime_parser_tell(parser);
		message = g_mime_parser_construct_message(parser);

		if (!message)
		{
			nFoundItems = -1;
			m_vItems.clear();
			break;
		}

		const char* strFrom = g_mime_message_get_sender(message);
		const char* strSubj = g_mime_message_get_subject(message);
		const char* strDate = g_mime_message_get_date_as_string(message);

		MBoxItem item = {0};
		item.StartPos = g_mime_parser_get_headers_begin(parser);
		item.EndPos = g_mime_parser_tell(parser);
		item.Sender = ConvertString(strFrom);
		item.Subject = ConvertString(strSubj);
		g_mime_message_get_date(message, &item.Date, &item.TimeZone);
		
		m_vItems.push_back(item);
		nFoundItems++;

		g_object_unref (message);
	}

	g_object_unref(stream);
	g_object_unref(parser);
	
	return nFoundItems;
}

bool CMboxReader::IsValidFile( FILE* f )
{
	char testBuf[100] = {0};
	
	int fSavePos = fseek(f, 0, SEEK_CUR); // Save file position
	char* retVal = fgets(testBuf, sizeof(testBuf), f);
	fseek(f, fSavePos, SEEK_SET); // Restore file position

	return (retVal != NULL) && (strncmp(testBuf, "From ", 5) == 0);
}

