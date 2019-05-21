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

	m_vItems.clear();
	while (!g_mime_parser_eos(parser))
	{
		msgStart = g_mime_parser_tell(parser);
		message = g_mime_parser_construct_message(parser);

		if (!message)
		{
			// If we could not parse next message then just exit and keep what we have
			break;
		}

		const char* strFrom = g_mime_message_get_sender(message);
		const char* strSubj = g_mime_message_get_subject(message);

		MBoxItem item;
		item.StartPos = g_mime_parser_get_headers_begin(parser);
		item.EndPos = g_mime_parser_tell(parser);
		item.Sender = ConvertString(strFrom);
		item.Subject = ConvertString(strSubj);
		item.IsDeleted = false;
		g_mime_message_get_date(message, &item.Date, &item.TimeZone);

		SanitizeString(item.Subject);
		
		m_vItems.push_back(item);

		g_object_unref (message);
	}

	g_object_unref(stream);
	g_object_unref(parser);
	
	return (int) m_vItems.size();
}

bool CMboxReader::IsValidFile( FILE* f )
{
	char testBuf[100] = {0};
	
	int fSavePos = fseek(f, 0, SEEK_CUR); // Save file position
	char* retVal = fgets(testBuf, sizeof(testBuf), f);
	fseek(f, fSavePos, SEEK_SET); // Restore file position

	return (retVal != NULL) && (strncmp(testBuf, "From ", 5) == 0);
}

