#include "stdafx.h"
#include "MboxReader.h"

bool CMboxReader::Open( const wchar_t* filepath )
{
	Close();

	FILE* fp;
	if (_wfopen_s(&fp, filepath, L"r") != 0)
		return false;

	char testBuf[100] = {0};
	if (!fgets(testBuf, sizeof(testBuf), fp) || strncmp(testBuf, "From ", 5) != 0)
	{
		fclose(fp);
		return false;
	}
	fseek(fp, 0, SEEK_SET); // Reset position to start of the file

	GMimeStream* stream;
	GMimeParser* parser;
	GMimeMessage* message;
	gint64 msgStart;

	stream = g_mime_stream_file_new(fp);
	g_mime_stream_file_set_owner((GMimeStreamFile*)stream, FALSE);
	
	parser = g_mime_parser_new_with_stream(stream);
	g_mime_parser_set_persist_stream(parser, TRUE);
	g_mime_parser_set_scan_from(parser, TRUE);

	bool fResult = true;
	while (!g_mime_parser_eos(parser))
	{
		msgStart = g_mime_parser_tell(parser);
		message = g_mime_parser_construct_message(parser);

		if (!message)
		{
			fResult = false;
			break;
		}

		const char* strFrom = g_mime_message_get_sender(message);
		const char* strSubj = g_mime_message_get_subject(message);
		const char* strDate = g_mime_message_get_date_as_string(message);

		MBoxItem item = {0};
		item.StartPos = msgStart;
		item.EndPos = g_mime_parser_tell(parser);
		item.Sender = (strFrom != NULL) ? g_mime_message_get_sender(message) : "";
		item.Subject = (strSubj != NULL) ? g_mime_message_get_subject(message) : "";
		item.Date = (strDate != NULL) ? g_mime_message_get_date_as_string(message) : "";
		
		vItems.push_back(item);

		g_object_unref (message);
	}

	g_object_unref(stream);
	g_object_unref(parser);
	
	if (fResult)
	{
		m_pFile = fp;
	}
	else
	{
		vItems.clear();
		fclose(fp);
	}
	
	return fResult;
}

void CMboxReader::Close()
{
	if (m_pFile) fclose(m_pFile);

	vItems.clear();
}

bool CMboxReader::Extract( int itemindex, const wchar_t* destpath )
{
	const MBoxItem& itemObj = vItems[itemindex];
	fseek(m_pFile, (size_t)itemObj.StartPos, SEEK_SET);

	size_t copySize = (size_t)(itemObj.EndPos - itemObj.StartPos);
	
	char* buf = (char*) malloc(copySize);
	fread(buf, 1, copySize, m_pFile);

	FILE* fd;
	_wfopen_s(&fd, destpath, L"wb");
	fwrite(buf, 1, copySize, fd);

	fclose(fd);	
	free(buf);

	return true;
}
