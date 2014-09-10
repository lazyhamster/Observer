#include "stdafx.h"
#include "SetupFactory8.h"
#include "Decomp.h"
#include "modulecrt/PEHelper.h"

#define SIGNATURE_SIZE 16
const uint8_t SIGNATURE[SIGNATURE_SIZE] = {0xe0,0xe0,0xe1,0xe1,0xe2,0xe2,0xe3,0xe3,0xe4,0xe4,0xe5,0xe5,0xe6,0xe6,0xe7,0xe7};

#define FILENAME_SIZE 264

#define FILENAME_EMBEDDED_INSTALLER "irsetup.exe"
#define FILENAME_LUA_DLL "lua5.1.dll"

#define STRBUF_SIZE(x) ( sizeof(x) / sizeof(x[0]) )

SetupFactory8::SetupFactory8( void )
{
	Init();
	m_nStartOffset = 0;
	m_nContentBaseOffset = 0;
}

SetupFactory8::~SetupFactory8( void )
{
	Close();
}

bool SetupFactory8::Open( CFileStream* inFile )
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
	
	// Check manifest for proper version
	std::string manifestText = GetManifest(inFile->FileName());
	if (manifestText.find("<description>Setup Factory 8.0 Run-time</description>") != std::string::npos)
	{
		m_pInFile = inFile;
		m_nStartOffset = inFile->GetPos();
		m_nVersion = 8;
		return true;
	}
	if (manifestText.find("<description>Setup Factory 9 Run-time</description>") != std::string::npos)
	{
		m_pInFile = inFile;
		m_nStartOffset = inFile->GetPos();
		m_nVersion = 9;
		return true;
	}

	return false;
}

void SetupFactory8::Close()
{
	m_nStartOffset = 0;
	m_nContentBaseOffset = 0;
	m_vFiles.clear();
	if (m_pInFile)
	{
		delete m_pInFile;
	}
	if (m_pScriptData)
	{
		delete m_pScriptData;
	}
	Init();
}

int SetupFactory8::EnumFiles()
{
	m_vFiles.clear();
	m_pInFile->SetPos(m_nStartOffset);

	// Read embedded installer .exe
	m_pInFile->Seek(10, STREAM_CURRENT);
	ReadSpecialFile(FILENAME_EMBEDDED_INSTALLER);

	// Read service files
	uint32_t numFiles;
	m_pInFile->ReadBuffer(&numFiles, sizeof(numFiles));

	// Check if this looks like real file number of we have Lua DLL file
	if (numFiles > 1000)
	{
		m_pInFile->Seek(-4, STREAM_CURRENT);
		ReadSpecialFile(FILENAME_LUA_DLL);
		m_pInFile->ReadBuffer(&numFiles, sizeof(numFiles));
	}

	char nameBuf[FILENAME_SIZE];
	int64_t fileSize;
	uint32_t fileCrc;

	for (uint32_t i = 0; i < numFiles; i++)
	{
		memset(nameBuf, 0, sizeof(nameBuf));
		if (!m_pInFile->ReadBuffer(nameBuf, FILENAME_SIZE) || !m_pInFile->ReadBuffer(&fileSize, sizeof(fileSize)) || !m_pInFile->ReadBuffer(&fileCrc, sizeof(fileCrc)))
			return -1;

		m_pInFile->Seek(4, STREAM_CURRENT);

		SFFileEntry fe = {0};
		strcpy_s(fe.LocalPath, STRBUF_SIZE(fe.LocalPath), nameBuf);
		fe.PackedSize = fileSize;
		fe.CRC = fileCrc;
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

		bool unpacked = false;

		if (m_eBaseCompression == COMP_UNKNOWN)
		{
			if (!DetectCompression(m_eBaseCompression))
				return -1;
		}

		switch(m_eBaseCompression)
		{
			case COMP_LZMA:
				unpacked = LzmaDecomp(m_pInFile, (uint32_t) fileSize, destUnpack, &destSize, &destCrc);
				break;
			case COMP_LZMA2:
				unpacked = Lzma2Decomp(m_pInFile, (uint32_t) fileSize, destUnpack, &destSize, &destCrc);
				break;
			case COMP_PKWARE:
				unpacked = Explode(m_pInFile, (uint32_t) fileSize, destUnpack, &destSize, &destCrc);
				break;
		}

		if (!unpacked) return -1;
		if (fileCrc != 0 && destCrc != fileCrc) return 1;

		fe.Compression = m_eBaseCompression;
		fe.UnpackedSize = destSize;
		m_vFiles.push_back(fe);
	}

	// No script == no other files
	if (m_pScriptData != nullptr)
	{
		// Now let's read actual content
		m_nContentBaseOffset = m_pInFile->GetPos();
		ParseScript(m_nContentBaseOffset);
	}
	
	return m_vFiles.size();
}

bool SetupFactory8::ExtractFile( int index, AStream* outStream )
{
	const SFFileEntry& entry = m_vFiles[index];
	if (entry.PackedSize == 0) return true;

	m_pInFile->SetPos(entry.DataOffset);

	uint32_t outCrc;
	bool ret = false;

	switch(entry.Compression)
	{
		case COMP_PKWARE:
			ret = Explode(m_pInFile, (uint32_t) entry.PackedSize, outStream, nullptr, &outCrc);
			break;
		case COMP_LZMA:
			ret = LzmaDecomp(m_pInFile, (uint32_t) entry.PackedSize, outStream, nullptr, &outCrc);
			break;
		case COMP_LZMA2:
			ret = Lzma2Decomp(m_pInFile, (uint32_t) entry.PackedSize, outStream, nullptr, &outCrc);
			break;
		case COMP_NONE:
			ret = Unstore(m_pInFile, (uint32_t) entry.PackedSize, outStream, &outCrc);
			break;
	}

	// Special treatment for irsetup.exe
	// First 2000 bytes of the file are xor-ed with 0x07
	if (ret && (strcmp(entry.LocalPath, FILENAME_EMBEDDED_INSTALLER) == 0))
	{
		uint8_t buf[2000];
		
		outStream->Seek(0, STREAM_BEGIN);
		outStream->ReadBuffer(buf, sizeof(buf));
		for (size_t i = 0; i < sizeof(buf); i++)
		{
			buf[i] = buf[i] ^ 0x07;
		}
		outStream->Seek(0, STREAM_BEGIN);
		outStream->WriteBuffer(buf, sizeof(buf));
	}

	return ret && (outCrc == entry.CRC || entry.CRC == 0);
}

int SetupFactory8::ParseScript( int64_t baseOffset )
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
	int64_t nDecompSize, nCompSize;
	uint32_t nCrc;
	uint16_t skipVal, packageNum;

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
		m_pScriptData->Seek(62, STREAM_CURRENT);
		ReadString(m_pScriptData, strDestDir); // Destination directory
		m_pScriptData->Seek(10, STREAM_CURRENT);
		SkipString(m_pScriptData); // Custom shortcut location
		SkipString(m_pScriptData); // Shortcut comment
		SkipString(m_pScriptData); // Shortcut description
		SkipString(m_pScriptData); // Shortcut startup arguments
		SkipString(m_pScriptData); // Shortcut start directory
		m_pScriptData->Seek(1, STREAM_CURRENT);
		SkipString(m_pScriptData); // Icon path
		m_pScriptData->Seek(12, STREAM_CURRENT);
		m_pScriptData->ReadBuffer(&nIsCompressed, sizeof(nIsCompressed));
		
		// A little bit of black magic (not sure if it is correct way)
		m_pScriptData->Seek(12, STREAM_CURRENT);
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

		SFFileEntry fe = {0};
		fe.PackedSize = nCompSize;
		fe.UnpackedSize = nDecompSize;
		fe.Compression = (nIsCompressed != 0) ? m_eBaseCompression : COMP_NONE;
		fe.DataOffset = nextOffset;
		fe.CRC = nCrc;

		strcpy_s(fe.LocalPath, MAX_PATH, strDestDir);
		if (strDestDir[0] && (strDestDir[strlen(strDestDir)-1] != '\\'))
		{
			strcat_s(fe.LocalPath, MAX_PATH, "\\");
		}
		strcat_s(fe.LocalPath, MAX_PATH, strBaseName);
		
		m_vFiles.push_back(fe);
		nextOffset += nCompSize;
		foundFiles++;
	}
	
	return foundFiles;
}

bool SetupFactory8::ReadSpecialFile( const char* fileName )
{
	int64_t fileSize = 0;
	m_pInFile->ReadBuffer(&fileSize, sizeof(fileSize));

	SFFileEntry fe = {0};
	strcpy_s(fe.LocalPath, STRBUF_SIZE(fe.LocalPath), fileName);
	fe.PackedSize = fileSize;
	fe.UnpackedSize = fileSize;
	fe.Compression = COMP_NONE;
	fe.DataOffset = m_pInFile->GetPos();

	m_vFiles.push_back(fe);

	return m_pInFile->Seek(fileSize, STREAM_CURRENT);
}

bool SetupFactory8::DetectCompression(EntryCompression &value)
{
	bool result = false;

	unsigned char buf[2];
	if (m_pInFile->ReadBuffer(buf, sizeof(buf)))
	{
		if (buf[0] == 0x00 && buf[1] == 0x06)
		{
			value = COMP_PKWARE;
			result = true;
		}
		else if (buf[0] == 0x5D && buf[1] == 0x00)
		{
			value = COMP_LZMA;
			result = true;
		}
		else if (buf[0] == 0x18)
		{
			value = COMP_LZMA2;
			result = true;
		}
		m_pInFile->Seek(-2, STREAM_CURRENT);
	}

	return result;
}

uint32_t SetupFactory8::FindFileBlockInScript()
{
	const char* srcStart = m_pScriptData->DataPtr();
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
