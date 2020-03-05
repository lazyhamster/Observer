// vdisk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <vcclr.h>
#include <msclr/marshal.h>
#include "ModuleDef.h"
#include "modulecrt/OptionsParser.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Reflection;
using namespace System::Text;
using namespace DiscUtils;

static int optDefaultCodepage = CP_OEMCP;
static bool optDisplayErrorPopups = true;

static void InitDiscUtils()
{
	Setup::SetupHelper::RegisterAssembly(Assembly::GetExecutingAssembly());
}

static void DisplayErrorMessage(String^ text, const wchar_t* caption)
{
	if (optDisplayErrorPopups)
	{
		msclr::interop::marshal_context ctx;
		MessageBox(0, ctx.marshal_as<const wchar_t*>(text), caption, MB_OK | MB_ICONERROR);
	}
}

ref class VDFileInfo
{
public:
	DiscFileSystemInfo^ Ref;
	int VolumeIndex;

	VDFileInfo(DiscFileSystemInfo^ objRef, int volIndex)
		: Ref(objRef), VolumeIndex(volIndex)
	{
		//
	}
};

ref class AssemblyLoaderEx
{
public:
	static Assembly^ AssemblyResolveEventHandler( Object^ sender, ResolveEventArgs^ args )
	{
		Assembly^ aLibSelf = Assembly::GetExecutingAssembly();
		if (args->RequestingAssembly == aLibSelf)
		{
			AssemblyName^ aName = gcnew AssemblyName(args->Name);
			String ^strTargetPath = IO::Path::GetDirectoryName(aLibSelf->Location) + "\\" + aName->Name + ".dll";

			if (IO::File::Exists(strTargetPath))
				return Assembly::LoadFile(strTargetPath);

			String^ msgText = String::Format("Dependency library not found\n{0}", args->Name);
			DisplayErrorMessage(msgText, L"VDISK Loading Error");
		}

		return nullptr;
	}

	static void UnhandledExceptionHandler( Object^ sender, UnhandledExceptionEventArgs^ args )
	{
		String^ msgText = String::Format("ACHTUNG!!!! DANGER!!!! : {0}", args->ExceptionObject);
		DisplayErrorMessage(msgText, L"Global Error");
	}
};

struct VDisk
{
	gcroot<VirtualDisk^> pVdiskObj;
	gcroot< List<VDFileInfo^>^ > vItems;
	gcroot< List<String^>^ > vVolLabels; // Cache for volume labels
};

static void EnumFilesInVolume(VDisk* vdObj, DiscDirectoryInfo^ dirInfo, LogicalVolumeInfo^ vol, int volIndex)
{
	try
	{
		array<DiscDirectoryInfo^> ^subDirList = dirInfo->GetDirectories();
		for (int i = 0; i < subDirList->Length; i++)
		{
			DiscDirectoryInfo^ subDir = subDirList[i];
			if (subDir->Name != "." && subDir->Name != ".." && subDir->Name->Length > 0 && subDir->Name[0] != 0)
			{
				vdObj->vItems->Add(gcnew VDFileInfo(subDir, volIndex));
				EnumFilesInVolume(vdObj, subDir, vol, volIndex);
			}
		}
	}
	catch (System::IO::IOException^)
	{
		//Usually this exception occurs on corrupted data
		//So we just skip all underlying directories
	}

	array<DiscFileInfo^> ^fileList = dirInfo->GetFiles();
	for (int i = 0; i < fileList->Length; i++)
	{
		VDFileInfo^ fileInfo = gcnew VDFileInfo(fileList[i], volIndex);
		vdObj->vItems->Add(fileInfo);
	}
}

static Encoding^ GetFileNameEncoding()
{
	try
	{
		int codePage;

		switch (optDefaultCodepage)
		{
		case CP_OEMCP:
			codePage = GetOEMCP();
			break;
		case CP_ACP:
			codePage = GetACP();
			break;
		default:
			codePage = optDefaultCodepage;
			break;
		}

		return Encoding::GetEncoding(codePage);
	}
	catch (NotSupportedException^)
	{
		return Encoding::Default;
	}
};

static void PrepareFileList(VDisk* vdisk)
{
	VolumeManager^ volm = gcnew VolumeManager(vdisk->pVdiskObj);

	array<LogicalVolumeInfo^> ^logvol = volm->GetLogicalVolumes();
	for (int i = 0; i < logvol->Length; i++)
	{
		LogicalVolumeInfo^ volInfo = logvol[i];
		vdisk->vVolLabels->Add("Volume_#" + i); // default volume name
		
		if (volInfo->Status != LogicalVolumeStatus::Healthy) continue;

		array<DiscUtils::FileSystemInfo^> ^fsinfo = FileSystemManager::DetectFileSystems(volInfo);
		if (fsinfo == nullptr || fsinfo->Length == 0) continue;

		FileSystemParameters^ fsParams = gcnew FileSystemParameters();
		fsParams->FileNameEncoding = GetFileNameEncoding();

		try
		{
			DiscFileSystem^ dfs = fsinfo[0]->Open(volInfo, fsParams);
			if (dfs->GetType() == Ntfs::NtfsFileSystem::typeid)
			{
				Ntfs::NtfsFileSystem^ ntfsFS = (Ntfs::NtfsFileSystem^)dfs;
				ntfsFS->NtfsOptions->HideHiddenFiles = false;
				ntfsFS->NtfsOptions->HideSystemFiles = false;
				//ntfsFS->NtfsOptions->HideMetafiles = false;
			}
			
			EnumFilesInVolume(vdisk, dfs->Root, volInfo, i);

			// Replace volume name with label if present
			// NOTE: C++/cli permits non-empty strings with 0 bytes inside, so we have to check for them too
			String^ volLabel = dfs->VolumeLabel->Trim();
			if (!String::IsNullOrEmpty(volLabel) && (volLabel->Length > 0) && (volLabel[0] != '\0'))
			{
				List<String^>^ labels = vdisk->vVolLabels;
				labels[i] = volLabel;
			}
		}
		catch (Exception^ ex)
		{
			String^ errText = String::Format("Volume listing error : {0}", ex);
			DisplayErrorMessage(errText, L"VDISK Error");
		}
	} // for
}

static ::FILETIME DateTimeToFileTime(DateTime dt)
{
	::FILETIME ft = {0};

	LARGE_INTEGER li;
	li.QuadPart = dt.ToFileTime();

	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;
	return ft;
}

const wchar_t* GetDiskType(VirtualDisk ^vdisk)
{
	if (vdisk->GetType() == Vmdk::Disk::typeid)
		return L"VMWare Virtual Hard Disk";
	else if (vdisk->GetType() == Vdi::Disk::typeid)
		return L"Virtual Box Hard Disk";
	else if (vdisk->GetType() == Vhd::Disk::typeid)
		return L"Microsoft Virtual Hard Disk";
	else if (vdisk->GetType() == Xva::Disk::typeid)
		return L"Xen Virtual Appliance Disk";
	else if (vdisk->GetType() == Vhdx::Disk::typeid)
		return L"Hyper-V virtual hard disk";
	else if (vdisk->GetType() == Dmg::Disk::typeid)
		return L"Apple Disk Image";

	switch(vdisk->DiskClass)
	{
	case VirtualDiskClass::FloppyDisk:
		return L"Floppy Disk Image";
	case VirtualDiskClass::OpticalDisk:
		return L"Optical Disk Image";
	case VirtualDiskClass::HardDisk:
		return L"Hard Disk Image";
	}

	return L"Virtual Disk";
}

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	String ^strPath = gcnew String(params.FilePath);
	VirtualDisk ^vdisk;
	try
	{
		vdisk = VirtualDisk::OpenDisk(strPath, IO::FileAccess::Read);
	}
	catch (Exception^)
	{
		vdisk = nullptr;
	}

	if (vdisk != nullptr)
	{
		bool fHaveValidVolumes = false;

		// Verify that we have any recognizable volumes
		VolumeManager^ volm = gcnew VolumeManager(vdisk);
		array<LogicalVolumeInfo^> ^logvol = volm->GetLogicalVolumes();
		for (int i = 0; i < logvol->Length; i++)
		{
			// Skip failed volumes
			if (logvol[i]->Status != LogicalVolumeStatus::Healthy) continue;
			
			array<DiscUtils::FileSystemInfo^> ^fsinfo = FileSystemManager::DetectFileSystems(logvol[i]);
			if (fsinfo != nullptr && fsinfo->Length > 0)
			{
				fHaveValidVolumes = true;
				break;
			}
		}

		if (fHaveValidVolumes)
		{
			VDisk* vdObj = new VDisk();
			vdObj->pVdiskObj = vdisk;
			vdObj->vItems = gcnew List<VDFileInfo^>();
			vdObj->vVolLabels = gcnew List<String^>();

			*storage = vdObj;

			memset(info, 0, sizeof(StorageGeneralInfo));
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, GetDiskType(vdisk));
			wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, L"-");
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"None");

			return SOR_SUCCESS;
		}
	}

	if (vdisk != nullptr)
	{
		delete vdisk;
		vdisk = nullptr;
	}

	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	VDisk* vdObj = (VDisk*) storage;
	if (vdObj != NULL)
	{
		vdObj->vVolLabels->Clear();
		vdObj->vVolLabels = nullptr;
		
		vdObj->vItems->Clear();
		vdObj->vItems = nullptr;

		delete vdObj->pVdiskObj;
		vdObj->pVdiskObj = nullptr;
		
		delete vdObj;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	VDisk* vdObj = (VDisk*) storage;
	if (vdObj == NULL) return FALSE;

	try
	{
		PrepareFileList(vdObj);
		return TRUE;
	}
	catch (Exception^ ex)
	{
		DisplayErrorMessage(ex->Message, L"VDISK Prepare Error");
		return FALSE;
	}
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	VDisk* vdObj = (VDisk*)storage;
	if (vdObj == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= vdObj->vItems->Count)
		return GET_ITEM_NOMOREITEMS;

	List<VDFileInfo^> ^fileList = vdObj->vItems;
	VDFileInfo^ fileInfo = fileList[item_index];

	List<String^> ^volLabels = vdObj->vVolLabels;
	String^ filePath = String::Format("[{0}]\\{1}", volLabels[fileInfo->VolumeIndex], fileInfo->Ref->FullName);

	// Remove trailing backslash if present
	if (filePath->EndsWith("\\"))
		filePath = filePath->Remove(filePath->Length - 1);

	// Helper class for String^ to wchar_t* conversion
	msclr::interop::marshal_context ctx;

	memset(item_info, 0, sizeof(StorageItemInfo));
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), ctx.marshal_as<const wchar_t*>(filePath));

	try
	{
		DiscFileSystemInfo^ fsData = fileInfo->Ref;

		item_info->Attributes = (DWORD)fsData->Attributes;
		item_info->Size = (fsData->GetType() == DiscFileInfo::typeid) ? ((DiscFileInfo^)fsData)->Length : 0;

		if (fsData->GetType() == DiscDirectoryInfo::typeid)
			item_info->Attributes |= FILE_ATTRIBUTE_DIRECTORY;

		try
		{
			item_info->CreationTime = DateTimeToFileTime(fsData->CreationTimeUtc);
			item_info->ModificationTime = DateTimeToFileTime(fsData->LastWriteTimeUtc);
		}
		catch (ArgumentOutOfRangeException^)
		{
			//Sometimes file has invalid date/time values
			//This info is not very crucial so let's just ignore the error
		}

		return GET_ITEM_OK;
	}
#ifdef _DEBUG
	catch (Exception^ ex)
	{
		if (optDisplayErrorPopups)
			DisplayErrorMessage(ex->Message, L"File parameters error");
#else
	catch (Exception^)
	{
#endif
		return GET_ITEM_ERROR;
	}
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	VDisk* vdObj = (VDisk*)storage;
	if (vdObj == NULL || params.ItemIndex < 0 || params.ItemIndex >= vdObj->vItems->Count || !params.DestPath)
		return SER_ERROR_SYSTEM;

	List<VDFileInfo^> ^fileList = vdObj->vItems;
	VDFileInfo^ fileInfo = fileList[params.ItemIndex];

	// We do not extract directories
	if (fileInfo->Ref->GetType() != DiscFileInfo::typeid)
		return SER_ERROR_SYSTEM;

	DiscFileInfo^ dfi = (DiscFileInfo^)fileInfo->Ref;
	if (!dfi->Exists) return SER_ERROR_READ;

	String^ strDestFile = gcnew String(params.DestPath);
	int result = SER_SUCCESS;

	try
	{
		IO::Stream ^inStr = dfi->OpenRead();
		IO::Stream ^outStr = gcnew IO::FileStream(strDestFile, IO::FileMode::Create, IO::FileAccess::Write, IO::FileShare::Read);

		try
		{
			int copyBufSize = 32 * 1024;
			array<Byte> ^copyBuf = gcnew array<Byte>(copyBufSize);

			Int64 bytesLeft = dfi->Length;
			while (bytesLeft > 0)
			{
				int copySize = (int) Math::Min(bytesLeft, (Int64) copyBufSize);
				int readNum = inStr->Read(copyBuf, 0, copySize);

				if (readNum != copySize)
				{
					result = SER_ERROR_READ;
					break;
				}

				outStr->Write(copyBuf, 0, readNum);
				bytesLeft -= readNum;

				if (params.Callbacks.FileProgress)
					if (!params.Callbacks.FileProgress(params.Callbacks.signalContext, readNum))
					{
						result = SER_USERABORT;
						break;
					}
			}
		}
		finally
		{
			outStr->Close();
			inStr->Close();
		}
	}
	catch (Exception^ ex)
	{
		if (String::Compare(ex->Source, "DiscUtils", true) == 0)
			result = SER_ERROR_READ;
		else
			result = SER_ERROR_WRITE;
	}

	if (result != SER_SUCCESS && IO::File::Exists(strDestFile))
	{
		IO::File::Delete(strDestFile);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

// {DF4155F8-753A-45CB-A27C-3EF864460D95}
static const GUID MODULE_GUID = { 0xdf4155f8, 0x753a, 0x45cb, { 0xa2, 0x7c, 0x3e, 0xf8, 0x64, 0x46, 0xd, 0x95 } };

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleId = MODULE_GUID;
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 1);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->ApiFuncs.OpenStorage = OpenStorage;
	LoadParams->ApiFuncs.CloseStorage = CloseStorage;
	LoadParams->ApiFuncs.GetItem = GetStorageItem;
	LoadParams->ApiFuncs.ExtractItem = ExtractItem;
	LoadParams->ApiFuncs.PrepareFiles = PrepareFiles;

	OptionsList opts(LoadParams->Settings);
	opts.GetValue(L"DefaultCodepage", optDefaultCodepage);
	opts.GetValue(L"DisplayErrorPopups", optDisplayErrorPopups);

	try
	{
		AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(AssemblyLoaderEx::AssemblyResolveEventHandler);
		AppDomain::CurrentDomain->UnhandledException += gcnew UnhandledExceptionEventHandler(AssemblyLoaderEx::UnhandledExceptionHandler);

		InitDiscUtils();

		return TRUE;
	}
	catch (Exception^ ex)
	{
		DisplayErrorMessage(ex->Message, L"Module Loading Error");

		return FALSE;
	}
}

void MODULE_EXPORT UnloadSubModule()
{
}
