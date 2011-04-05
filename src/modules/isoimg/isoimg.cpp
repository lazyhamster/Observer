// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "iso_tc.h"
#include "iso_ext.h"
#include "OptionsParser.h"

static int g_DefaultCharser = CP_ACP;
static BOOL g_UseRockRidge = TRUE;

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	bool needPasswd = false;
	IsoImage* image = GetImage(params.FilePath, params.Password, needPasswd);
	if (!image) return needPasswd ? SOR_PASSWORD_REQUIRED : SOR_INVALID_FILE;

	image->DefaultCharset = g_DefaultCharser;
	image->UseRockRidge = g_UseRockRidge;

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
		
	*storage = image;

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
	{
		IsoImage* img = (IsoImage*) storage;
		FreeImage(img);
	}
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

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->OpenStorage = OpenStorage;
	LoadParams->CloseStorage = CloseStorage;
	LoadParams->GetItem = GetStorageItem;
	LoadParams->ExtractItem = ExtractItem;

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"Charset", g_DefaultCharser);
	opts.GetValue(L"RockRidge", g_UseRockRidge);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
