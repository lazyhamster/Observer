#include "stdafx.h"
#include "HWClassFactory.h"

#include "HW2BigFile.h"
#include "HW1BigFile.h"
#include "SgaFile.h"

CHWAbstractStorage* CHWClassFactory::LoadFile( const wchar_t *FilePath )
{
	CBasicFile* pFile = new CBasicFile(FilePath);
	if (!pFile->Open())
	{
		delete pFile;
		return NULL;
	}
	
	CHWAbstractStorage *reader = NULL;

	if (!reader)
	{
		reader = new CHW2BigFile();
		if (!reader->Open(pFile))
		{
			delete reader;
			reader = NULL;
		}
	}

	if (!reader)
	{
		reader = new CHW1BigFile();
		if (!reader->Open(pFile))
		{
			delete reader;
			reader = NULL;
		}
	}

	if (!reader)
	{
		reader = new CSgaFile();
		if (!reader->Open(pFile))
		{
			delete reader;
			reader = NULL;
		}
	}
	
	if (!reader)
		delete pFile;
	
	return reader;
}
