// isoimg.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"
#include "iso_tc.h"
#include "iso_ext.h"
#include "modulecrt/OptionsParser.h"

static int g_DefaultCharset = CP_ACP;
static bool g_UseRockRidge = true;

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	bool needPasswd = false;
	IsoImage* image = GetImage(params.FilePath, params.Password, needPasswd, false);
	if (!image) return needPasswd ? SOR_PASSWORD_REQUIRED : SOR_INVALID_FILE;

	image->DefaultCharset = g_DefaultCharset;
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
		switch (image->ImageType)
		{
			case ISOTYPE_ISZ:
				wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"ISZ");
				break;
			default:
				wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"ISO");
				break;
		}

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

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return TRUE;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	IsoImage* image = (IsoImage*) storage;
	if (!image) return GET_ITEM_ERROR;

	if ((item_index < 0) || (item_index >= (int) image->DirectoryCount))
		return GET_ITEM_NOMOREITEMS;

	Directory dir = image->DirectoryList[item_index];

	memset(item_info, 0, sizeof(StorageItemInfo));
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), dir.FilePath);
	item_info->Attributes = GetDirectoryAttributes(&dir);

	if (dir.VolumeDescriptor->XBOX)
	{
		item_info->Size = (DWORD) dir.XBOXRecord.DataLength;
	}
	else
	{
		item_info->Size = (dir.Record.FileFlags & FATTR_DIRECTORY)? 0 : (DWORD) dir.Record.DataLength;

		FILETIME ftime = VolumeDateTimeToFileTime(dir.Record.RecordingDateAndTime);
		
		item_info->ModificationTime = ftime;
		item_info->CreationTime = ftime;
	}

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	IsoImage* image = (IsoImage*) storage;
	if (!image) return SER_ERROR_SYSTEM;

	if (params.ItemIndex < 0 || params.ItemIndex >= (int) image->DirectoryCount)
		return SER_ERROR_SYSTEM;

	Directory dir = image->DirectoryList[params.ItemIndex];
	if (!IsDirectory(&dir))
	{
		int nExtractRes = ExtractFile(image, &dir, params.DestPath, &(params.Callbacks));

		return nExtractRes;
	}
	
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {CD7BA4E7-A4B4-41AB-8A60-FA692621DA2F}
static const GUID MODULE_GUID = { 0xcd7ba4e7, 0xa4b4, 0x41ab, { 0x8a, 0x60, 0xfa, 0x69, 0x26, 0x21, 0xda, 0x2f } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 2);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"Charset", g_DefaultCharset);
	opts.GetValue(L"RockRidge", g_UseRockRidge);

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
