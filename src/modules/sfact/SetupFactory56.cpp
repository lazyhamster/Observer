#include "StdAfx.h"
#include "SetupFactory56.h"
#include "Decomp.h"

#define DATA_OFFSET_5 0x8000
#define DATA_OFFSET_6 0x12000

#define SCRIPT_FILE "irsetup.dat"

#define FILENAME_SIZE_5 16
#define FILENAME_SIZE_6 260

#define SIGNATURE_SIZE 8
const uint8_t SIGNATURE[SIGNATURE_SIZE] = {0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7};

#define STRBUF_SIZE(x) ( sizeof(x) / sizeof(x[0]) )

SetupFactory56::SetupFactory56(void)
{
	m_pInFile = nullptr;
	m_nVersion = 0;
	m_nStartOffset = 0;
	m_pScriptData = nullptr;
}

SetupFactory56::~SetupFactory56(void)
{
	Close();
}

bool SetupFactory56::Open( CFileStream* inFile )
{
	Close();
	
	inFile->SetPos(0);

	if (CheckSignature(inFile, DATA_OFFSET_5))
	{
		m_nVersion = 5;
		m_nStartOffset = DATA_OFFSET_5 + SIGNATURE_SIZE;
		m_pInFile = inFile;
		return true;
	}

	if (CheckSignature(inFile, DATA_OFFSET_6))
	{
		m_nVersion = 6;
		m_nStartOffset = DATA_OFFSET_6 + SIGNATURE_SIZE;
		m_pInFile = inFile;
		return true;
	}

	return false;
}

void SetupFactory56::Close()
{
	m_nVersion = 0;
	m_nStartOffset = 0;
	m_vFiles.clear();
	if (m_pInFile)
	{
		delete m_pInFile;
		m_pInFile = nullptr;
	}
	if (m_pScriptData)
	{
		delete m_pScriptData;
		m_pScriptData = nullptr;
	}
}

int SetupFactory56::EnumFiles()
{
	m_vFiles.clear();
	m_pInFile->SetPos(m_nStartOffset);

	// Read service files
	uint32_t numFiles;
	m_pInFile->ReadBuffer(&numFiles, sizeof(numFiles));

	uint32_t size, crc;
	char nameBuf[FILENAME_SIZE_6];
	size_t fileNameSize = 0;

	switch(m_nVersion)
	{
		case 5:
			fileNameSize = FILENAME_SIZE_5;
			break;
		case 6:
			fileNameSize = FILENAME_SIZE_6;
			break;
		default:
			return -1;
	}

	for (uint32_t i = 0; i < numFiles; i++)
	{
		memset(nameBuf, 0, sizeof(nameBuf));
		if (!m_pInFile->ReadBuffer(nameBuf, fileNameSize) || !m_pInFile->ReadBuffer(&size, sizeof(size)) || !m_pInFile->ReadBuffer(&crc, sizeof(crc)))
			return -1;
		
		SFFileEntry fe = {0};
		MultiByteToWideChar(CP_ACP, 0, nameBuf, -1, fe.LocalPath, STRBUF_SIZE(fe.LocalPath));
		fe.PackedSize = size;
		fe.CRC = crc;
		fe.IsCompressed = true;
		fe.DataOffset = m_pInFile->GetPos();
		
		bool isScript = strcmp(nameBuf, SCRIPT_FILE) == 0;
		AStream* destUnpack;
		uint32_t destSize, destCrc;
		
		if (isScript)
		{
			destUnpack = new CMemoryStream();
			m_pScriptData = (CMemoryStream*) destUnpack;
		}
		else
		{
			destUnpack = new CNullStream();
		}

		if (!Explode(m_pInFile, size, destUnpack, &destSize, &destCrc))
			return false;

		fe.UnpackedSize = destSize;
		m_vFiles.push_back(fe);
	}

	// No script == no other files
	if (m_pScriptData == nullptr)
		return m_vFiles.size();

	// Now let's read actual content
	ParseScript(m_pInFile->GetPos());

	return m_vFiles.size();
}

bool SetupFactory56::CheckSignature( CFileStream* inFile, int64_t offset )
{
	uint8_t sigBuf[SIGNATURE_SIZE];
	return inFile->SetPos(offset)
		&& inFile->ReadBuffer(sigBuf, SIGNATURE_SIZE)
		&& (memcmp(sigBuf, SIGNATURE, SIGNATURE_SIZE) == 0);
}

bool SetupFactory56::ExtractFile( int index, AStream* outStream )
{
	const SFFileEntry& entry = m_vFiles[index];
	if (entry.PackedSize == 0) return true;

	m_pInFile->SetPos(entry.DataOffset);

	uint32_t outCrc;
	bool ret = false;

	if (entry.IsCompressed)
		ret = Explode(m_pInFile, (uint32_t) entry.PackedSize, outStream, nullptr, &outCrc);
	else
		ret = Unstore(m_pInFile, (uint32_t) entry.PackedSize, outStream, &outCrc);

	return ret && (outCrc == entry.CRC || entry.CRC == 0);
}

int SetupFactory56::ParseScript(int64_t baseOffset)
{
	int foundFiles = 0;
	m_pScriptData->SetPos(0);

	uint16_t numEntries;
	m_pScriptData->ReadBuffer(&numEntries, sizeof(numEntries));
	m_pScriptData->Seek(4, STREAM_CURRENT);  // Skip 2 unknown uint16_t numbers

	uint16_t classNameLen;
	char className[128] = {0};
	m_pScriptData->ReadBuffer(&classNameLen, sizeof(classNameLen));
	m_pScriptData->ReadBuffer(className, min(classNameLen, sizeof(className) - 1));

	// Check if we have proper script
	if (strcmp(className, "CFileInfo") != 0) return -1;

	char strBaseName[MAX_PATH];
	char strDestDir[MAX_PATH];
	uint8_t nIsCompressed;
	uint32_t nDecompSize, nCompSize, nCrc;

	int64_t nextOffset = baseOffset;
		
	for (uint16_t i = 0; i < numEntries; i++)
	{
		if (m_nVersion == 6)
		{
			uint32_t unknown1;
			m_pScriptData->ReadBuffer(&unknown1, sizeof(unknown1));
		}

		SkipString(m_pScriptData); // Full source path
		ReadString(m_pScriptData, strBaseName); // Base name
		SkipString(m_pScriptData); // Source directory
		SkipString(m_pScriptData); // Suffix
		m_pScriptData->Seek(1, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&nDecompSize, sizeof(nDecompSize));
		m_pScriptData->Seek(38, STREAM_CURRENT);
		ReadString(m_pScriptData, strDestDir); // Destination directory
		m_pScriptData->Seek(5, STREAM_CURRENT);
		SkipString(m_pScriptData);
		m_pScriptData->Seek(9, STREAM_CURRENT);
		SkipString(m_pScriptData);
		m_pScriptData->Seek(5, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&nIsCompressed, sizeof(nIsCompressed));

		switch(m_nVersion)
		{
			case 5:
				m_pScriptData->Seek(17, STREAM_CURRENT);
				break;
			case 6:
				m_pScriptData->Seek(8, STREAM_CURRENT);
				SkipString(m_pScriptData);
				m_pScriptData->Seek(2, STREAM_CURRENT);
				break;
		}

		m_pScriptData->ReadBuffer(&nCompSize, sizeof(nCompSize));
		m_pScriptData->ReadBuffer(&nCrc, sizeof(nCrc));
		m_pScriptData->Seek(39, STREAM_CURRENT);

		SFFileEntry fe = {0};
		fe.PackedSize = nCompSize;
		fe.UnpackedSize = nDecompSize;
		fe.IsCompressed = nIsCompressed != 0;
		fe.DataOffset = nextOffset;
		fe.CRC = nCrc;

		char fullLocalPath[MAX_PATH] = {0};
		strcpy_s(fullLocalPath, MAX_PATH, strDestDir);
		if (fullLocalPath[0] && (fullLocalPath[strlen(fullLocalPath)-1] != '\\'))
		{
			strcat_s(fullLocalPath, MAX_PATH, "\\");
		}
		strcat_s(fullLocalPath, MAX_PATH, strBaseName);
		
		MultiByteToWideChar(CP_ACP, 0, fullLocalPath, -1, fe.LocalPath, STRBUF_SIZE(fe.LocalPath));
		
		m_vFiles.push_back(fe);
		nextOffset += nCompSize;
		foundFiles++;
	}

	return foundFiles;
}

bool SetupFactory56::SkipString( AStream* stream )
{
	uint8_t strSize;
	return stream->ReadBuffer(&strSize, sizeof(strSize)) && stream->Seek(strSize, STREAM_CURRENT);
}

bool SetupFactory56::ReadString( AStream* stream, char* buf )
{
	uint8_t strSize;
	bool retval = stream->ReadBuffer(&strSize, sizeof(strSize)) && stream->ReadBuffer(buf, strSize);
	if (retval) buf[strSize] = 0;
	return retval;
}
