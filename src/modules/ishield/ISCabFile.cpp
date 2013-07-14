#include "stdafx.h"
#include "ISCabFile.h"

#include "IS5\IS5CabFile.h"
#include "IS6\IS6CabFile.h"

ISCabFile* OpenCab(const wchar_t* filePath)
{
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	
	IS5::IS5CabFile* is5 = new IS5::IS5CabFile();
	if (is5->Open(hFile))
	{
		return is5;
	}
	delete is5;

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	IS6::IS6CabFile* is6 = new IS6::IS6CabFile();
	if (is6->Open(hFile))
	{
		return is6;
	}
	delete is6;

	CloseHandle(hFile);
	return NULL;
}

void CloseCab(ISCabFile* cab)
{
	if (cab != NULL)
	{
		cab->Close();
		delete cab;
	}
}
