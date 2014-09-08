#include "stdafx.h"
#include "SetupFactory8.h"
#include "Decomp.h"
#include "modulecrt/PEHelper.h"

#define SIGNATURE_SIZE 16
const uint8_t SIGNATURE[SIGNATURE_SIZE] = {0xe0,0xe0,0xe1,0xe1,0xe2,0xe2,0xe3,0xe3,0xe4,0xe4,0xe5,0xe5,0xe6,0xe6,0xe7,0xe7};

#define FILENAME_SIZE 264

#define FILENAME_EMBEDDED_INSTALLER L"irsetup.exe"
#define FILENAME_LUA_DLL L"lua5.1.dll"

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

	// Check manifest for proper version
	std::string manifestText = GetManifest(inFile->FileName());
	if (manifestText.find("<description>Setup Factory 8.0 Run-time</description>") == std::string::npos)
		return false;

	inFile->SetPos(ovlStart);

	uint8_t sigBuf[SIGNATURE_SIZE];
	if (inFile->ReadBuffer(sigBuf, sizeof(sigBuf)) && (memcmp(sigBuf, SIGNATURE, SIGNATURE_SIZE) == 0))
	{
		m_pInFile = inFile;
		m_nStartOffset = inFile->GetPos();
		m_nVersion = 8;

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
		MultiByteToWideChar(m_nFilenameCodepage, 0, nameBuf, -1, fe.LocalPath, STRBUF_SIZE(fe.LocalPath));
		fe.PackedSize = fileSize;
		fe.CRC = fileCrc;
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

		if (!Explode(m_pInFile, (uint32_t) fileSize, destUnpack, &destSize, &destCrc))
			return -1;

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

	if (entry.IsCompressed)
		ret = Explode(m_pInFile, (uint32_t) entry.PackedSize, outStream, nullptr, &outCrc);
	else
		ret = Unstore(m_pInFile, (uint32_t) entry.PackedSize, outStream, &outCrc);

	// Special treatment for irsetup.exe
	// First 2000 bytes of the file are xor-ed with 0x07
	if (ret && (wcscmp(entry.LocalPath, FILENAME_EMBEDDED_INSTALLER) == 0))
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
	m_pScriptData->SetPos(0);
	
	//TODO: implement
	return foundFiles;
}

bool SetupFactory8::ReadSpecialFile( const wchar_t* fileName )
{
	int64_t fileSize = 0;
	m_pInFile->ReadBuffer(&fileSize, sizeof(fileSize));

	SFFileEntry fe = {0};
	wcscpy_s(fe.LocalPath, STRBUF_SIZE(fe.LocalPath), fileName);
	fe.PackedSize = fileSize;
	fe.UnpackedSize = fileSize;
	fe.IsCompressed = false;
	fe.DataOffset = m_pInFile->GetPos();

	m_vFiles.push_back(fe);

	return m_pInFile->Seek(fileSize, STREAM_CURRENT);
}
