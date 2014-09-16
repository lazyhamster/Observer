#include "stdafx.h"
#include "SfFile.h"

#include "SetupFactory56.h"
#include "SetupFactory8.h"

SetupFactoryFile* OpenInstaller( const wchar_t* filePath )
{
	CFileStream* inFile = CFileStream::Open(filePath, true, false);
	if (!inFile) return nullptr;

	SetupFactory56* sf56 = new SetupFactory56();
	if (sf56->Open(inFile))
		return sf56;
	delete sf56;

	SetupFactory8* sf8 = new SetupFactory8();
	if (sf8->Open(inFile))
		return sf8;
	delete sf8;

	delete inFile;
	return nullptr;
}

void FreeInstaller( SetupFactoryFile* file )
{
	if (file)
	{
		file->Close();
		delete file;
	}
}

bool SetupFactoryFile::SkipString( AStream* stream )
{
	uint8_t strSize;
	return stream->ReadBuffer(&strSize, sizeof(strSize)) && stream->Seek(strSize, STREAM_CURRENT);
}

bool SetupFactoryFile::ReadString( AStream* stream, char* buf )
{
	uint8_t strSize;
	bool retval = stream->ReadBuffer(&strSize, sizeof(strSize)) && stream->ReadBuffer(buf, strSize);
	if (retval) buf[strSize] = 0;
	return retval;
}

void SetupFactoryFile::Init()
{
	m_pInFile = nullptr;
	m_nFilenameCodepage = CP_ACP;
	m_nVersion = 0;
	m_pScriptData = nullptr;
	m_eBaseCompression = COMP_UNKNOWN;
	m_nStartOffset = 0;
}

void SetupFactoryFile::Close()
{
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
