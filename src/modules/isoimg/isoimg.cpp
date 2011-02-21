// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
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

template <typename T>
void TrimRight(T* buf, size_t bufSize)
{
	int i = (int) bufSize - 1;
	while ((i >= 0) && (buf[i] == 32))
	{
		buf[i] = 0;
		i--;
	}
}

FILETIME StringToTime(const char* data)
{
	FILETIME res = {0};
	
	SYSTEMTIME st = {0};
	int valnum = sscanf(data, "%4d%2d%2d%2d%2d%2d", &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
	if (valnum >= 3) SystemTimeToFileTime(&st, &res);
	
	return res;
}

//////////////////////////////////////////////////////////////////////////

int ExtractFile(IsoImage *image, Directory *dir, const wchar_t *destPath, const ExtractProcessCallbacks* epc);
FILETIME VolumeDateTimeToFileTime(VolumeDateTime &vdtime);

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	IsoImage* image = GetImage(params.FilePath);
	if (!image) return SOR_INVALID_FILE;

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
		return SOR_INVALID_FILE;
	}
		
	*storage = (INT_PTR *) image;

	if (image->VolumeDescriptors && image->VolumeDescriptors->XBOX)
	{
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"XBOX ISO");
		wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"MICROSOFT*XBOX*MEDIA");
		info->Created = image->VolumeDescriptors->XBOXVolumeDescriptor.FileTime;
	}
	else
	{
		wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"ISO");
		info->Created = StringToTime((char*)image->VolumeDescriptors->VolumeDescriptor.VolumeCreationDateAndTime);

		if (image->VolumeDescriptors->Unicode)
		{
			wchar_t* vd = (wchar_t*) (image->VolumeDescriptors->VolumeDescriptor.VolumeIdentifier + 1);
			wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, vd);
			TrimRight<wchar_t>(info->Comment, wcslen(info->Comment));
		}
		else
		{
			char *vd = (char*) image->VolumeDescriptors->VolumeDescriptor.VolumeIdentifier;
			TrimRight<char>(vd, 32);
			MultiByteToWideChar(CP_ACP, 0, vd, (int) strlen(vd), info->Comment, STORAGE_PARAM_MAX_LEN);
		}
	}
	wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");
	
	return SOR_SUCCESS;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	if (storage)
		FreeImage((IsoImage*) storage);
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	IsoImage* image = (IsoImage*) storage;
	if (!image) return GET_ITEM_ERROR;

	if ((item_index < 0) || (item_index >= (int) image->DirectoryCount))
		return GET_ITEM_NOMOREITEMS;

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

		FILETIME ftime = VolumeDateTimeToFileTime(dir.Record.RecordingDateAndTime);
		
		item_data->ftLastWriteTime = ftime;
		item_data->ftCreationTime = ftime;
	}

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	IsoImage* image = (IsoImage*) storage;
	if (!image) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= (int) image->DirectoryCount)
		return SER_ERROR_SYSTEM;

	Directory dir = image->DirectoryList[params.item];
	if (!IsDirectory(&dir))
	{
		int nExtractRes = ExtractFile(image, &dir, params.destFilePath, &(params.callbacks));

		return nExtractRes;
	}
	
	return SER_ERROR_SYSTEM;
}
