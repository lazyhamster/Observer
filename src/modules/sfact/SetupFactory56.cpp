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
		MultiByteToWideChar(CP_ACP, 0, nameBuf, -1, fe.LocalPath, sizeof(fe.LocalPath) / sizeof(fe.LocalPath[0]));
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
	m_pInFile->SetPos(entry.DataOffset);
	uint32_t outCrc;
	bool ret = Explode(m_pInFile, (uint32_t) entry.PackedSize, outStream, nullptr, &outCrc);
	return ret && (outCrc == entry.CRC || entry.CRC == 0);
}
