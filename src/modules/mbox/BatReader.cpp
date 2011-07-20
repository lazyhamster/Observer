#include "stdafx.h"
#include "BatReader.h"

#define TBB_FILE_MAGIC "\x20\x06\x79\x19"
#define TBB_MESSAGE_MAGIC "\x21\x09\x70\x19"

#define GLOBAL_HEADER_SIZE 3084

#pragma pack(push, 1)

struct TbbMessageRec
{
	char magic[4];				// magic number
	uint32_t headerSize;
	uint32_t unknown1;
	uint32_t receivedTime;		// received time (unix timestamp) (little endian! 15, 14, 13, 12)
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

bool CBatReader::IsValidFile( FILE* f )
{
	char fileMagic[4] = {0};
	if (fread(fileMagic, 1, 4, f) != sizeof(fileMagic) || strncmp(fileMagic, TBB_FILE_MAGIC, 4) != 0)
		return false;

	uint16_t fileHeaderSize;
	if (fread(&fileHeaderSize, 2, 1, f) != 1)
		return false;

	if (fseek(f, fileHeaderSize, SEEK_SET) != 0)
		return false;

	m_nDataStartPos = fileHeaderSize;
	return true;
}

int CBatReader::Scan()
{
	if (!m_pSrcFile || m_nDataStartPos <= 0) return -1;

	_fseeki64(m_pSrcFile, m_nDataStartPos, SEEK_SET);
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

		MBoxItem item;
		item.StartPos = _ftelli64(m_pSrcFile);
		item.EndPos = item.StartPos + msgHeader.dataSize;
		item.Sender = L"";
		item.Subject = L"";
		item.Date = msgHeader.receivedTime;
		item.IsDeleted = (msgHeader.statusFlag & 1) != 0;

		m_vItems.push_back(item);
		nFoundItems++;

		// Move to next message
		if (_fseeki64(m_pSrcFile, msgHeader.dataSize, SEEK_CUR) != 0)
			break;
	}
	
	return nFoundItems;
}
