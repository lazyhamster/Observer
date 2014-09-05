#include "stdafx.h"
#include "SfFile.h"
#include "SetupFactory56.h"

SetupFactoryFile* OpenInstaller( const wchar_t* filePath )
{
	CFileStream* inFile = CFileStream::Open(filePath, true, false);
	if (!inFile) return nullptr;

	SetupFactory56* sf56 = new SetupFactory56();
	if (sf56->Open(inFile))
		return sf56;
	delete sf56;

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
