#include "stdafx.h"
#include "SetupFactory7.h"
#include "Decomp.h"
#include "modulecrt/PEHelper.h"

#define SIGNATURE_SIZE 8
const uint8_t SIGNATURE[SIGNATURE_SIZE] = {0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7};

#define FILENAME_SIZE 260

#define FILENAME_EMBEDDED_INSTALLER "irsetup.exe"

SetupFactory7::SetupFactory7( void )
{
	Init();
}

SetupFactory7::~SetupFactory7( void )
{
	Close();
}

bool SetupFactory7::Open( CFileStream* inFile )
{
	Close();

	int64_t ovlStart, ovlSize;
	if (!FindFileOverlay(inFile, ovlStart, ovlSize))
		return false;

	inFile->SetPos(ovlStart);

	// Check signature
	uint8_t sigBuf[SIGNATURE_SIZE];
	if (!inFile->ReadBuffer(sigBuf, sizeof(sigBuf)) || (memcmp(sigBuf, SIGNATURE, SIGNATURE_SIZE) != 0))
		return false;

	// In version 7 after signature there is XOR-ed irsetup.exe, so next number would be file size
	// In versions 5/6 it is number of files - small number
	uint32_t testNum;
	if (inFile->Read(&testNum) && (testNum > 100))
	{
		// Return stream pointer
		inFile->Seek(-4, STREAM_CURRENT);

		m_pInFile = inFile;
		m_nStartOffset = inFile->GetPos();
		m_nVersion = 7;
		m_eBaseCompression = COMP_PKWARE;
		return true;
	}

	return false;
}

int SetupFactory7::EnumFiles()
{
	m_vFiles.clear();
	m_pInFile->SetPos(m_nStartOffset);

	// Read embedded installer .exe
	//TODO: some file do not have 1 byte shift, investigate
	m_pInFile->Seek(1, STREAM_CURRENT);
	ReadSpecialFile(FILENAME_EMBEDDED_INSTALLER, true);

	// Read service files
	uint32_t numFiles;
	m_pInFile->ReadBuffer(&numFiles, sizeof(numFiles));

	uint32_t size, crc;
	char nameBuf[FILENAME_SIZE];

	for (uint32_t i = 0; i < numFiles; i++)
	{
		memset(nameBuf, 0, sizeof(nameBuf));
		if (!m_pInFile->ReadBuffer(nameBuf, FILENAME_SIZE) || !m_pInFile->ReadBuffer(&size, sizeof(size)) || !m_pInFile->ReadBuffer(&crc, sizeof(crc)))
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

int SetupFactory7::ParseScript( int64_t baseOffset )
{
	int foundFiles = 0;

	uint32_t filesBlockOff = FindFileBlockInScript();
	if (filesBlockOff == 0)
		return 0;

	m_pScriptData->SetPos(filesBlockOff);

	uint16_t numEntries;
	m_pScriptData->ReadBuffer(&numEntries, sizeof(numEntries));
	m_pScriptData->Seek(4, STREAM_CURRENT);  // Skip 2 unknown uint16_t numbers, always 0xFFFF and 0x0001

	uint16_t classNameLen;
	char className[128] = {0};
	m_pScriptData->ReadBuffer(&classNameLen, sizeof(classNameLen));
	m_pScriptData->ReadBuffer(className, min(classNameLen, sizeof(className) - 1));

	// Check if we have proper script
	if (strcmp(className, "CSetupFileData") != 0) return -1;

	char strBaseName[MAX_PATH];
	char strDestDir[MAX_PATH];
	uint8_t nIsCompressed;
	uint32_t nDecompSize, nCompSize;
	uint32_t nCrc;
	uint16_t skipVal, packageNum;
	uint8_t origAttr, useOrigAttr, forcedAttr;
	//int64_t modTime, createTime;

	int64_t nextOffset = baseOffset;
	m_pScriptData->Seek(5, STREAM_CURRENT);

	for (uint16_t i = 0; i < numEntries; i++)
	{
		SkipString(m_pScriptData); // Full source path
		ReadString(m_pScriptData, strBaseName); // Base name
		SkipString(m_pScriptData); // Source directory
		SkipString(m_pScriptData); // Suffix
		SkipString(m_pScriptData); // Run-time folder (usually 'Archive')
		SkipString(m_pScriptData); // File description
		m_pScriptData->Seek(2, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&nDecompSize, sizeof(nDecompSize));
		m_pScriptData->ReadBuffer(&origAttr, sizeof(origAttr)); // Attributes of the original source file
		m_pScriptData->Seek(37, STREAM_CURRENT);
		ReadString(m_pScriptData, strDestDir); // Destination directory
		m_pScriptData->Seek(10, STREAM_CURRENT);
		SkipString(m_pScriptData); // Custom shortcut location
		SkipString(m_pScriptData); // Shortcut comment
		SkipString(m_pScriptData); // Shortcut description
		SkipString(m_pScriptData); // Shortcut startup arguments
		SkipString(m_pScriptData); // Shortcut start directory
		m_pScriptData->Seek(1, STREAM_CURRENT);
		SkipString(m_pScriptData); // Icon path
		m_pScriptData->Seek(8, STREAM_CURRENT);
		SkipString(m_pScriptData); // Font reg name (if file is font)
		m_pScriptData->Seek(3, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&nIsCompressed, sizeof(nIsCompressed));
		m_pScriptData->ReadBuffer(&useOrigAttr, sizeof(origAttr)); // 1 - use original file attributes
		m_pScriptData->ReadBuffer(&forcedAttr, sizeof(forcedAttr)); // Set this attributes if prev. value is 0

		// A little bit of black magic (not sure if it is correct way)
		m_pScriptData->Seek(10, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&skipVal, sizeof(skipVal));
		m_pScriptData->Seek(skipVal * 2, STREAM_CURRENT);

		SkipString(m_pScriptData); // Script condition
		m_pScriptData->Seek(2, STREAM_CURRENT);
		SkipString(m_pScriptData); // Install type
		SkipString(m_pScriptData);
		m_pScriptData->ReadBuffer(&packageNum, sizeof(packageNum));
		for (uint16_t i = 0; i < packageNum; i++)
		{
			SkipString(m_pScriptData); // Package name
		}
		SkipString(m_pScriptData); // File notes
		m_pScriptData->ReadBuffer(&nCompSize, sizeof(nCompSize));
		m_pScriptData->ReadBuffer(&nCrc, sizeof(nCrc));
		m_pScriptData->Seek(8, STREAM_CURRENT);

		SFFileEntry fe;
		fe.PackedSize = nCompSize;
		fe.UnpackedSize = nDecompSize;
		fe.Compression = (nIsCompressed != 0) ? m_eBaseCompression : COMP_NONE;
		fe.DataOffset = nextOffset;
		fe.CRC = nCrc;
		fe.Attributes = useOrigAttr ? origAttr : forcedAttr;
		//fe.LastWriteTime = modTime;
		//fe.CreationTime = createTime;
		fe.LocalPath = JoinLocalPath(strDestDir, strBaseName);

		m_vFiles.push_back(fe);
		nextOffset += nCompSize;
		foundFiles++;
	}

	return foundFiles;
}

bool SetupFactory7::ReadSpecialFile( const char* fileName, bool isXORed )
{
	uint32_t fileSize = 0;
	m_pInFile->ReadBuffer(&fileSize, sizeof(fileSize));

	SFFileEntry fe;
	fe.LocalPath = fileName;
	fe.PackedSize = fileSize;
	fe.UnpackedSize = fileSize;
	fe.Compression = COMP_NONE;
	fe.DataOffset = m_pInFile->GetPos();
	fe.IsXORed = isXORed;

	m_vFiles.push_back(fe);

	return m_pInFile->Seek(fileSize, STREAM_CURRENT);
}

uint32_t SetupFactory7::FindFileBlockInScript()
{
	const char* srcStart = m_pScriptData->CDataPtr();
	const char* srcEnd = srcStart + m_pScriptData->GetSize();
	const char* ptrnStart = "CSetupFileData";
	const char* ptrnEnd = ptrnStart + strlen(ptrnStart);

	const char* res = std::search(srcStart, srcEnd, ptrnStart, ptrnEnd);

	if (res != srcEnd)
	{
		return (uint32_t) (res - srcStart) - 8;
	}

	return 0;
}
