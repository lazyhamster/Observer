#include "stdafx.h"
#include "SfFile.h"
#include "Decomp.h"

#include "SetupFactory56.h"
#include "SetupFactory7.h"
#include "SetupFactory8.h"

template <typename T>
static SetupFactoryFile* TryOpen(CFileStream* pFile)
{
	T* obj = new T();
	if (obj->Open(pFile))
		return obj;

	delete obj;
	return nullptr;
}

#define RNN(type, file) { auto sf = TryOpen<type>(file); if (sf) return sf; }

SetupFactoryFile* OpenInstaller( const wchar_t* filePath )
{
	CFileStream* inFile = CFileStream::Open(filePath, true, false);
	if (!inFile) return nullptr;

	RNN(SetupFactory56, inFile);
	RNN(SetupFactory7, inFile);
	RNN(SetupFactory8, inFile);

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

std::string SetupFactoryFile::JoinLocalPath(const char* dirName, const char* fileName)
{
	auto dirLen = dirName ? strlen(dirName) : 0;
	
	std::string strResult;
	if (dirLen > 0)
	{
		strResult = dirName;
		if (dirName[dirLen - 1] != '\\')
			strResult += '\\';
	}
	strResult += fileName;
	
	return strResult;
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

bool SetupFactoryFile::ExtractFile( int index, AStream* outStream )
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
	if (ret && entry.IsXORed)
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
