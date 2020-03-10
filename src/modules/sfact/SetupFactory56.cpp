#include "StdAfx.h"
#include "SetupFactory56.h"
#include "Decomp.h"

#define FILENAME_SIZE_5 16
#define FILENAME_SIZE_6 260

#define SIGNATURE_SIZE 8
const uint8_t SIGNATURE[SIGNATURE_SIZE] = {0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7};

SetupFactory56::SetupFactory56(void)
{
	Init();
}

SetupFactory56::~SetupFactory56(void)
{
	Close();
}

bool SetupFactory56::CheckSignature( CFileStream* inFile, int64_t offset, int sufVersion )
{
	uint8_t sigBuf[SIGNATURE_SIZE];
	bool isMatch = inFile->SetPos(offset)
		&& inFile->ReadBuffer(sigBuf, SIGNATURE_SIZE)
		&& (memcmp(sigBuf, SIGNATURE, SIGNATURE_SIZE) == 0);

	m_nVersion = sufVersion;
	m_nStartOffset = offset + SIGNATURE_SIZE;

	return isMatch;
}

bool SetupFactory56::Open( CFileStream* inFile )
{
	Close();
	
	inFile->SetPos(0);

	if (CheckSignature(inFile, 0x8000, 5)
		|| CheckSignature(inFile, 0x12000, 6) // Main offset for version 6
		|| CheckSignature(inFile, 0x15000, 6) // Alternative offset for version 6 (usually have no files in script)
		|| CheckSignature(inFile, 0x1A000, 6) // Another offset (also usually without files)
		)
	{
		m_pInFile = inFile;
		m_eBaseCompression = COMP_PKWARE;
		return true;
	}

	return false;
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
		
		SFFileEntry fe;
		fe.LocalPath = nameBuf;
		fe.PackedSize = size;
		fe.CRC = crc;
		fe.Compression = COMP_PKWARE;
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
			return -1;

		fe.UnpackedSize = destSize;
		m_vFiles.push_back(fe);
	}

	// No script == no other files
	if (m_pScriptData != nullptr)
	{
		// Now let's read actual content
		int64_t contentBaseOffset = m_pInFile->GetPos();
		ParseScript(contentBaseOffset);
	}

	return (int) m_vFiles.size();
}

int SetupFactory56::ParseScript(int64_t baseOffset)
{
	int foundFiles = 0;
	m_pScriptData->SetPos(0);

	uint16_t numEntries;
	m_pScriptData->ReadBuffer(&numEntries, sizeof(numEntries));
	m_pScriptData->Seek(4, STREAM_CURRENT);  // Skip 2 unknown uint16_t numbers, always 0xFFFF and 0x0003

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
	uint8_t origAttr, useOrigAttr, forcedAttr;
	int32_t modTime, createTime;

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
		m_pScriptData->ReadBuffer(&origAttr, sizeof(origAttr)); // Attributes of the original source file
		m_pScriptData->ReadBuffer(&createTime, sizeof(createTime));
		m_pScriptData->Seek(4, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&modTime, sizeof(modTime));
		m_pScriptData->Seek(25, STREAM_CURRENT);
		ReadString(m_pScriptData, strDestDir); // Destination directory
		m_pScriptData->Seek(5, STREAM_CURRENT);
		SkipString(m_pScriptData); // Shortcut description
		SkipString(m_pScriptData); // Shortcut startup arguments
		SkipString(m_pScriptData); // Shortcut working directory
		m_pScriptData->Seek(1, STREAM_CURRENT);
		SkipString(m_pScriptData); // Path to icon
		m_pScriptData->Seek(5, STREAM_CURRENT);
		SkipString(m_pScriptData); // Package name
		m_pScriptData->Seek(1, STREAM_CURRENT);
		SkipString(m_pScriptData); // Font name
		m_pScriptData->Seek(3, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&nIsCompressed, sizeof(nIsCompressed));
		m_pScriptData->ReadBuffer(&useOrigAttr, sizeof(origAttr)); // 1 - use original file attributes
		m_pScriptData->ReadBuffer(&forcedAttr, sizeof(forcedAttr)); // Set this attributes if prev. value is 0

		switch(m_nVersion)
		{
			case 5:
				m_pScriptData->Seek(15, STREAM_CURRENT);
				break;
			case 6:
				m_pScriptData->Seek(6, STREAM_CURRENT);
				SkipString(m_pScriptData);
				m_pScriptData->Seek(2, STREAM_CURRENT);
				break;
		}

		m_pScriptData->ReadBuffer(&nCompSize, sizeof(nCompSize));
		m_pScriptData->ReadBuffer(&nCrc, sizeof(nCrc));
		m_pScriptData->Seek(39, STREAM_CURRENT);

		SFFileEntry fe;
		fe.PackedSize = nCompSize;
		fe.UnpackedSize = nDecompSize;
		fe.Compression = (nIsCompressed != 0) ? COMP_PKWARE : COMP_NONE;
		fe.DataOffset = nextOffset;
		fe.CRC = nCrc;
		fe.Attributes = useOrigAttr ? origAttr : forcedAttr;
		fe.LastWriteTime = modTime;
		fe.CreationTime = createTime;
		fe.LocalPath = JoinLocalPath(strDestDir, strBaseName);
		
		m_vFiles.push_back(fe);
		nextOffset += nCompSize;
		foundFiles++;
	}

	return foundFiles;
}
