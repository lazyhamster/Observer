#include "stdafx.h"
#include "BatReader.h"

#define TBB_FILE_MAGIC "\x20\x06\x79\x19"
#define TBB_MESSAGE_MAGIC "\x21\x09\x70\x19"

#pragma pack(push, 1)

struct TbbMessageRec
{
	char magic[4];				// magic number
	uint32_t headerSize;
	uint32_t unknown1;
	uint32_t receivedTime;		// received time (Unix timestamp) (little endian! 15, 14, 13, 12)
	uint16_t id;				// id number (maybe display position) (little endian! 17, 16)
	uint16_t unknown2;
	uint8_t statusFlag;
	uint8_t unknown3[7];
	uint32_t colorGroup;		// message belongs to a certain color group
	uint32_t priorityStatus;
	uint32_t dataSize;			// size of the variable part (little endian! 39, 38, 37, 36)
	uint8_t unknown4[8];
};

#pragma pack(pop)

int CBatReader::Scan()
{
	if (!m_pSrcFile) return -1;

	char fileMagic[4] = { 0 };
	if (fread(fileMagic, 1, sizeof(fileMagic), m_pSrcFile) != sizeof(fileMagic) || strncmp(fileMagic, TBB_FILE_MAGIC, 4) != 0)
		return -1;

	uint16_t fileHeaderSize;
	if (fread(&fileHeaderSize, sizeof(fileHeaderSize), 1, m_pSrcFile) != 1)
		return -1;

	if (_fseeki64(m_pSrcFile, fileHeaderSize, SEEK_SET) != 0)
		return -1;

	GMimeStream* stream = g_mime_stream_file_new(m_pSrcFile);
	g_mime_stream_file_set_owner((GMimeStreamFile*)stream, FALSE);

	GMimeParserOptions* parserOpts = g_mime_parser_options_new();

	int nFoundItems = 0;

	m_vItems.clear();
	while (!feof(m_pSrcFile))
	{
		TbbMessageRec msgHeader = {0};
		size_t readSize = fread(&msgHeader, 1, sizeof(msgHeader), m_pSrcFile);
		if (readSize != sizeof(msgHeader))
			break;

		if (strncmp(msgHeader.magic, TBB_MESSAGE_MAGIC, 4) || msgHeader.headerSize != sizeof(msgHeader))
			break;

		__int64 msgStart = _ftelli64(m_pSrcFile);
		__int64 msgEnd = msgStart + msgHeader.dataSize;

		GMimeParser* parser = g_mime_parser_new_with_stream(stream);
		g_mime_parser_set_persist_stream(parser, TRUE);
		g_mime_parser_set_format(parser, GMIME_FORMAT_MESSAGE);
		
		g_mime_stream_set_bounds(stream, msgStart, msgEnd);
		GMimeMessage* message = g_mime_parser_construct_message(parser, parserOpts);

		const char* strFrom = nullptr;
		const char* strSubj = nullptr;

		if (message)
		{
			strFrom = GetSenderAddress(message);
			strSubj = g_mime_message_get_subject(message);
		}

		MBoxItem item;
		item.StartPos = msgStart;
		item.EndPos = msgEnd;
		item.Sender = strFrom ? ConvertString(strFrom) : L"Unknown";
		item.Subject = strSubj ? ConvertString(strSubj) : L"NOT_PARSED";
		item.DateUtc = msgHeader.receivedTime;  //TODO: check if it's actually UTC
		item.IsDeleted = (msgHeader.statusFlag & 1) != 0;

		// Subject need sanitizing because it will be a base for file name
		SanitizeString(item.Subject);

		if (message) g_object_unref(message);
		g_object_unref(parser);

		// Move to next message
		if (_fseeki64(m_pSrcFile, msgEnd, SEEK_SET) != 0)
			break;

		m_vItems.push_back(item);
		nFoundItems++;
	}

	g_mime_parser_options_free(parserOpts);
	g_object_unref(stream);

	return nFoundItems;
}

bool CBatReader::CheckSample(const void* sampleBuffer, size_t sampleSize)
{
	return (sampleSize > 4) && (strncmp((const char*)sampleBuffer, TBB_FILE_MAGIC, 4) == 0);
}
