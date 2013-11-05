#include "stdafx.h"
#include "SfFile.h"
#include "SetupFactory56.h"

SetupFactoryFile* OpenInstaller( const wchar_t* filePath )
{
	CFileStream* inFile = new CFileStream(filePath, true, false);
	if (!inFile->IsValid())	return nullptr;

	SetupFactory56* sf56 = new SetupFactory56();
	if (sf56->Open(inFile))
		return sf56;
	delete sf56;

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
