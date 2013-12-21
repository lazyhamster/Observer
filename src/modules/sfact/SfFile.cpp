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
