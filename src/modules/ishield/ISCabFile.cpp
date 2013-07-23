#include "stdafx.h"
#include "ISCabFile.h"
#include "Utils.h"

#include "IS5\IS5CabFile.h"
#include "IS6\IS6CabFile.h"
#include "ISU\ISUCabFile.h"

ISCabFile* OpenCab(const wchar_t* filePath)
{
	HANDLE hFile = OpenFileForRead(filePath);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	
	IS5::IS5CabFile* is5 = new IS5::IS5CabFile();
	if (is5->Open(hFile, filePath))
	{
		return is5;
	}
	delete is5;

	IS6::IS6CabFile* is6 = new IS6::IS6CabFile();
	if (is6->Open(hFile, filePath))
	{
		return is6;
	}
	delete is6;

	ISU::ISUCabFile* isu = new ISU::ISUCabFile();
	if (isu->Open(hFile, filePath))
	{
		return isu;
	}
	delete isu;

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

bool ISCabFile::Open( HANDLE headerFile, const wchar_t* heaerFilePath )
{
	SetFilePointer(headerFile, 0, NULL, FILE_BEGIN);
	
	if (InternalOpen(headerFile))
	{
		m_hHeaderFile = headerFile;
		m_sCabPattern = GenerateCabPatern(heaerFilePath);
		GenerateInfoFile();
		return true;
	}

	return false;
}
