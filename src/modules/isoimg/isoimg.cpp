// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\ModuleDef.h"
#include "iso_tc.h"

bool IsDirectory(Directory *dir)
{
	if (dir->VolumeDescriptor->XBOX)
	{
		return (dir->XBOXRecord.FileFlags & XBOX_DIRECTORY) > 0;
	}
	else
	{
		return (dir->Record.FileFlags & FATTR_DIRECTORY) > 0;
	}
}

DWORD GetSize(Directory *dir)
{
	if (dir->VolumeDescriptor->XBOX)
		return (DWORD) dir->XBOXRecord.DataLength;
	else
		return (DWORD) dir->Record.DataLength;
}

DWORD GetDirectoryAttributes(Directory* dir)
{
	DWORD dwAttr = 0;

	if (dir->VolumeDescriptor->XBOX)
	{
		if (dir->XBOXRecord.FileFlags & XBOX_DIRECTORY)
			dwAttr |= FILE_ATTRIBUTE_DIRECTORY;
		if (dir->XBOXRecord.FileFlags & XBOX_HIDDEN)
			dwAttr |= FILE_ATTRIBUTE_HIDDEN;
		if (dir->XBOXRecord.FileFlags & XBOX_ARCHIVE)
			dwAttr |= FILE_ATTRIBUTE_ARCHIVE;
		if (dir->XBOXRecord.FileFlags & XBOX_SYSTEM)
			dwAttr |= FILE_ATTRIBUTE_SYSTEM;
		
		if (dwAttr == 0)
			dwAttr = FILE_ATTRIBUTE_NORMAL;
	}
	else
	{
		dwAttr = FILE_ATTRIBUTE_ARCHIVE;
		if (dir->Record.FileFlags & FATTR_DIRECTORY)
			dwAttr |= FILE_ATTRIBUTE_DIRECTORY;
		if (dir->Record.FileFlags & FATTR_HIDDEN)
			dwAttr |= FILE_ATTRIBUTE_HIDDEN;
	}

	return dwAttr;
}

//////////////////////////////////////////////////////////////////////////

int ExtractFile(IsoImage *image, Directory *dir, const wchar_t *destPath, const ExtractProcessCallbacks* epc);

//////////////////////////////////////////////////////////////////////////

struct IsoListContextInfo
{
	IsoImage* storage;	// pointer to image
	wchar_t* path;		// path to dir for which list is searched
	int itemIndex;		// last index of listed item, we start search from it
};

int MODULE_EXPORT LoadSubModule(int Reserved)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	IsoImage* image = GetImage(path);
	if (!image) return FALSE;

	DWORD count = 0;
	if( LoadAllTrees( image, 0, &count, true ) && count )
	{
		int size = sizeof( Directory ) * count;
		image->DirectoryList = (Directory*)malloc( size );
		ZeroMemory(image->DirectoryList, size);
		LoadAllTrees( image, &image->DirectoryList, &image->DirectoryCount, true );
	}

	if ( (image->Index >= image->DirectoryCount) || !count )
	{
		FreeImage(image);
		return FALSE;
	}
		
	*storage = (INT_PTR *) image;

	wcscpy(info->Format, L"ISO");
	info->NumRealItems = image->DirectoryCount;
	if (image->VolumeDescriptors && image->VolumeDescriptors->XBOX)
		wcscpy(info->SubType, L"XBOX");
	else
		wcscpy(info->SubType, L"PC");

	return TRUE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	if (storage)
		FreeImage((IsoImage*) storage);
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	IsoImage* image = (IsoImage*) storage;
	if (!image) return FALSE;

	if ((item_index < 0) || (item_index >= (int) image->DirectoryCount))
		return FALSE;

	Directory dir = image->DirectoryList[item_index];

	wcscpy_s(item_path, path_size, dir.FilePath);
	
	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, dir.FileName);
	wcscpy_s(item_data->cAlternateFileName, 14, L"");
	item_data->dwFileAttributes = GetDirectoryAttributes(&dir);

	if (dir.VolumeDescriptor->XBOX)
	{
		item_data->nFileSizeLow = (DWORD) dir.XBOXRecord.DataLength;
	}
	else
	{
		item_data->nFileSizeLow = (dir.Record.FileFlags & FATTR_DIRECTORY)? 0 : (DWORD) dir.Record.DataLength;

		SYSTEMTIME time;
		FILETIME   ftime;
		ZeroMemory( &time, sizeof( time ) );
		time.wYear   = (WORD)(dir.Record.RecordingDateAndTime.Year + 1900);
		time.wMonth  = dir.Record.RecordingDateAndTime.Month;
		time.wDay    = dir.Record.RecordingDateAndTime.Day;
		time.wHour   = dir.Record.RecordingDateAndTime.Hour;
		time.wMinute = dir.Record.RecordingDateAndTime.Minute;
		time.wSecond = dir.Record.RecordingDateAndTime.Second;
		SystemTimeToFileTime( &time, &ftime );
		item_data->ftLastWriteTime = ftime;
	}

	return TRUE;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	IsoImage* image = (IsoImage*) storage;
	if (!image) return SER_ERROR_SYSTEM;

	size_t nItemLen = wcslen(params.item);
	for (DWORD i = 0; i < image->DirectoryCount; i++)
	{
		Directory dir = image->DirectoryList[i];

		bool fIsDir = IsDirectory(&dir);
		if (!fIsDir && (wcscmp(dir.FilePath, params.item) == 0))
		{
			return ExtractFile(image, &dir, params.destPath, &(params.Callbacks));
		}
	}
	
	return SER_ERROR_SYSTEM;
}
