#include "stdafx.h"
#include "HWClassFactory.h"

#include "HW2BigFile.h"

CHWAbstractStorage* CHWClassFactory::LoadFile( const wchar_t *FilePath )
{
	HANDLE hFile = CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	CHWAbstractStorage *reader = NULL;

	if (!reader)
	{
		reader = new CHW2BigFile();
		if (!reader->Open(hFile))
		{
			delete reader;
			reader = NULL;
		}
	}
	
	return reader;
}
