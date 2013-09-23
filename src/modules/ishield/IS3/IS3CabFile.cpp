#include "stdafx.h"
#include "IS3CabFile.h"
#include "Utils.h"

namespace IS3
{

IS3CabFile::IS3CabFile()
{

}

IS3CabFile::~IS3CabFile()
{

}

bool IS3CabFile::InternalOpen( HANDLE headerFile )
{
	if (headerFile == INVALID_HANDLE_VALUE)
		return false;

	CABHEADER cabHeader = {0};

	if (!ReadBuffer(headerFile, &cabHeader, sizeof(cabHeader)))
		return false;

	// Validate file

	if (cabHeader.Signature != Z_SIG)
		return false;
	if (cabHeader.NumFiles == 0)
		return false;
	if (cabHeader.ArcSize != FileSize(headerFile) || cabHeader.offsEntries >= cabHeader.ArcSize)
		return false;

	if (!SeekFile(headerFile, cabHeader.offsEntries))
		return false;

	for (int i = 0; i < cabHeader.NumFiles; i++)
	{
		//
	}
	
	return false;
}

void IS3CabFile::Close()
{

}

int IS3CabFile::GetTotalFiles() const
{
	return 0;
}

bool IS3CabFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	return false;
}

int IS3CabFile::ExtractFile( int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx )
{
	return CAB_EXTRACT_READ_ERR;
}

} //namespace IS3
