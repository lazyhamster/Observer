// vdisk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <vcclr.h>
#include <msclr/marshal.h>
#include "ModuleDef.h"
#include "modulecrt/OptionsParser.h"
#include "FatNameEncoding.h"
#include "ExtFeatureFilterStream.h"
#include "ExtDirectoryReader.h"
#include "NtfsDirectoryReader.h"

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
	FatNameEncoding^ NameEncoding; // Set only for volumes which use internal name representation
	ItemProperties^ Properties; // Set only if item properties were read during listing
	ExtDirectoryReader^ ExtReader; // Set only for items on ext volumes, used for extraction

	VDFileInfo(DiscFileSystemInfo^ objRef, int volIndex, FatNameEncoding^ nameEncoding)
		: Ref(objRef), VolumeIndex(volIndex), NameEncoding(nameEncoding)
	{
		//
	}

	VDFileInfo(DiscFileSystemInfo^ objRef, int volIndex, ItemProperties^ properties, ExtDirectoryReader^ extReader)
		: Ref(objRef), VolumeIndex(volIndex), Properties(properties), ExtReader(extReader)
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

	// Files list reading progress
	const ListProgressCallbacks* pProgress = nullptr;
	__int64 nItemsTotal = 0; // Estimate, 0 if unknown
	bool fAborted = false;
};

// Returns false if listing should stop
static bool ReportListProgress(VDisk* vdObj)
{
	if (vdObj->fAborted) return false;

	const ListProgressCallbacks* progress = vdObj->pProgress;
	if (progress && progress->ListProgress)
	{
		// Callback limits redraw rate on its own, but there is no need to call it for every item
		int count = vdObj->vItems->Count;
		if ((count % 256) == 0 && !progress->ListProgress(progress->signalContext, count, vdObj->nItemsTotal))
			vdObj->fAborted = true;
	}
	return !vdObj->fAborted;
}

static void EnumFilesInVolume(VDisk* vdObj, DiscDirectoryInfo^ dirInfo, LogicalVolumeInfo^ vol, int volIndex, FatNameEncoding^ nameEncoding)
{
	try
	{
		array<DiscDirectoryInfo^> ^subDirList = dirInfo->GetDirectories();
		for (int i = 0; i < subDirList->Length; i++)
		{
			DiscDirectoryInfo^ subDir = subDirList[i];
			if (subDir->Name != "." && subDir->Name != ".." && subDir->Name->Length > 0 && subDir->Name[0] != 0)
			{
				vdObj->vItems->Add(gcnew VDFileInfo(subDir, volIndex, nameEncoding));
				if (!ReportListProgress(vdObj)) return;
				EnumFilesInVolume(vdObj, subDir, vol, volIndex, nameEncoding);
				if (vdObj->fAborted) return;
			}
		}
	}
	catch (Exception^)
	{
		//Usually this exception occurs on corrupted data
		//So we just skip all underlying directories
	}

	try
	{
		array<DiscFileInfo^> ^fileList = dirInfo->GetFiles();
		for (int i = 0; i < fileList->Length; i++)
		{
			VDFileInfo^ fileInfo = gcnew VDFileInfo(fileList[i], volIndex, nameEncoding);
			vdObj->vItems->Add(fileInfo);
			if (!ReportListProgress(vdObj)) return;
		}
	}
	catch (Exception^)
	{
		//Unreadable directory should not prevent listing of the rest of the volume
	}
}

// Same as EnumFilesInVolume, but item properties are read together with directories
static void EnumCachedFilesInVolume(VDisk* vdObj, DiscFileSystem^ dfs, IDirectoryReader^ reader, ExtDirectoryReader^ extReader, String^ dirPath, int volIndex, int depth)
{
	const int MaxDepth = 256; // Protection from loops in corrupted file systems
	if (depth > MaxDepth) return;

	List<ItemProperties^>^ entries = gcnew List<ItemProperties^>();
	try
	{
		reader->ReadDirectory(dirPath, entries);
	}
	catch (Exception^)
	{
		//Unreadable directory should not prevent listing of the rest of the volume
		return;
	}

	// Directories first, then files, same order as in EnumFilesInVolume
	String^ prefix = (dirPath->Length > 0) ? dirPath + "\\" : String::Empty;
	for each (ItemProperties^ entry in entries)
	{
		if (!entry->IsDirectory) continue;

		String^ path = prefix + entry->Name;
		vdObj->vItems->Add(gcnew VDFileInfo(dfs->GetDirectoryInfo(path), volIndex, entry, extReader));
		if (!ReportListProgress(vdObj)) return;
		EnumCachedFilesInVolume(vdObj, dfs, reader, extReader, path, volIndex, depth + 1);
		if (vdObj->fAborted) return;
	}

	for each (ItemProperties^ entry in entries)
	{
		if (entry->IsDirectory) continue;
		vdObj->vItems->Add(gcnew VDFileInfo(dfs->GetFileInfo(prefix + entry->Name), volIndex, entry, extReader));
		if (!ReportListProgress(vdObj)) return;
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

static array<Byte>^ ReadVolumeBytes(IO::Stream^ volume, __int64 offset, int count)
{
	array<Byte>^ data = gcnew array<Byte>(count);
	volume->Position = offset;
	int done = 0;
	while (done < count)
	{
		int n = volume->Read(data, done, count - done);
		if (n <= 0) return nullptr;
		done += n;
	}
	return data;
}

// Number of items for progress display, it doesn't have to be exact. Returns 0 if unknown.
static __int64 EstimateItemsCount(DiscFileSystem^ dfs, IO::Stream^ volume)
{
	try
	{
		if (dfs->GetType() == Ext::ExtFileSystem::typeid)
		{
			// Used inodes, except reserved ones (they are below s_first_ino)
			array<Byte>^ sb = ReadVolumeBytes(volume, 1024, 0x58);
			if (sb == nullptr) return 0;
			__int64 used = (__int64)BitConverter::ToUInt32(sb, 0x00) - BitConverter::ToUInt32(sb, 0x10);
			__int64 reserved = (BitConverter::ToUInt32(sb, 0x4C) >= 1) ? BitConverter::ToUInt32(sb, 0x54) - 1 : 10;
			return Math::Max(used - reserved, (__int64)0);
		}
		if (dfs->GetType() == Ntfs::NtfsFileSystem::typeid)
		{
			// Records in MFT, including free ones, so this is usually too much
			array<Byte>^ boot = ReadVolumeBytes(volume, 0, 0x42);
			if (boot == nullptr) return 0;
			int clusterSize = BitConverter::ToUInt16(boot, 0x0B) * boot[0x0D];
			int recordValue = (SByte)boot[0x40];
			int recordSize = (recordValue < 0) ? (1 << -recordValue) : recordValue * clusterSize;
			if (recordSize <= 0) return 0;
			return Math::Max(dfs->GetFileLength("$MFT") / recordSize - 16, (__int64)0); // First 16 are metafiles
		}
	}
	catch (Exception^)
	{
		// No estimate then
	}
	return 0;
}

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

		FatNameEncoding^ fatNameEncoding = nullptr;
		if (fsinfo[0]->Name == "FAT")
		{
			fatNameEncoding = gcnew FatNameEncoding(fsParams->FileNameEncoding);
			fsParams->FileNameEncoding = fatNameEncoding;
		}

		try
		{
			DiscFileSystem^ dfs;
			ExtDirectoryReader^ extReader = nullptr;
			NtfsDirectoryReader^ ntfsReader = nullptr;
			if (fsinfo[0]->Name == "ext")
			{
				IO::Stream^ extVolume = gcnew ExtFeatureFilterStream(volInfo->Open());
				dfs = gcnew Ext::ExtFileSystem(extVolume, fsParams);
				extReader = gcnew ExtDirectoryReader(dfs, extVolume);
			}
			else
			{
				dfs = fsinfo[0]->Open(volInfo, fsParams);
			}

			if (dfs->GetType() == Ntfs::NtfsFileSystem::typeid)
			{
				Ntfs::NtfsFileSystem^ ntfsFS = (Ntfs::NtfsFileSystem^)dfs;
				ntfsFS->NtfsOptions->HideHiddenFiles = false;
				ntfsFS->NtfsOptions->HideSystemFiles = false;
				//ntfsFS->NtfsOptions->HideMetafiles = false;

				// Created after options are set, they affect which entries are listed
				ntfsReader = gcnew NtfsDirectoryReader(ntfsFS);
			}
			
			if (fatNameEncoding != nullptr)
				fatNameEncoding->LoadLongNames(volInfo->Open());

			if (vdisk->pProgress)
			{
				__int64 estimate = EstimateItemsCount(dfs, volInfo->Open());
				vdisk->nItemsTotal = (estimate > 0) ? vdisk->vItems->Count + estimate : 0;
			}

			if (extReader != nullptr && extReader->IsAvailable)
				EnumCachedFilesInVolume(vdisk, dfs, extReader, extReader, String::Empty, i, 0);
			else if (ntfsReader != nullptr && ntfsReader->IsAvailable)
				EnumCachedFilesInVolume(vdisk, dfs, ntfsReader, nullptr, String::Empty, i, 0);
			else
				EnumFilesInVolume(vdisk, dfs->Root, volInfo, i, fatNameEncoding);

			// Replace volume name with label if present
			// NOTE: C++/cli permits non-empty strings with 0 bytes inside, so we have to check for them too
			String^ volLabel = dfs->VolumeLabel;
			if (fatNameEncoding != nullptr)
				volLabel = fatNameEncoding->ToDisplayPath(volLabel, false);
			volLabel = volLabel->Trim();
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

		if (vdisk->fAborted) break;
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

int MODULE_EXPORT PrepareFilesEx(HANDLE storage, const ListProgressCallbacks* progress)
{
	VDisk* vdObj = (VDisk*) storage;
	if (vdObj == NULL) return FALSE;

	vdObj->pProgress = progress;
	vdObj->nItemsTotal = 0;
	vdObj->fAborted = false;
	try
	{
		PrepareFileList(vdObj);

		// Exact number at the end, plugin uses it as total when it reads the items
		if (progress && !vdObj->fAborted && !progress->ListProgress(progress->signalContext, vdObj->vItems->Count, vdObj->vItems->Count))
			vdObj->fAborted = true;

		vdObj->pProgress = nullptr;
		return vdObj->fAborted ? FALSE : TRUE;
	}
	catch (Exception^ ex)
	{
		vdObj->pProgress = nullptr;
		DisplayErrorMessage(ex->Message, L"VDISK Prepare Error");
		return FALSE;
	}
}

int MODULE_EXPORT PrepareFiles(HANDLE storage)
{
	return PrepareFilesEx(storage, nullptr);
}

static String^ GetItemDisplayPath(VDisk* vdObj, VDFileInfo^ fileInfo, bool useLongNames)
{
	String^ itemPath = fileInfo->Ref->FullName;
	if (fileInfo->NameEncoding != nullptr)
		itemPath = fileInfo->NameEncoding->ToDisplayPath(itemPath, useLongNames);

	List<String^> ^volLabels = vdObj->vVolLabels;
	String^ filePath = String::Format("[{0}]\\{1}", volLabels[fileInfo->VolumeIndex], itemPath);

	// Remove trailing backslash if present
	if (filePath->EndsWith("\\"))
		filePath = filePath->Remove(filePath->Length - 1);

	return filePath;
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	VDisk* vdObj = (VDisk*)storage;
	if (vdObj == NULL || item_index < 0) return GET_ITEM_ERROR;

	if (item_index >= vdObj->vItems->Count)
		return GET_ITEM_NOMOREITEMS;

	List<VDFileInfo^> ^fileList = vdObj->vItems;
	VDFileInfo^ fileInfo = fileList[item_index];

	// Long names can make path too large for the output buffer
	String^ filePath = GetItemDisplayPath(vdObj, fileInfo, true);
	if (filePath->Length >= (int)STRBUF_SIZE(item_info->Path))
		filePath = GetItemDisplayPath(vdObj, fileInfo, false);
	if (filePath->Length >= (int)STRBUF_SIZE(item_info->Path))
		return GET_ITEM_ERROR;

	// Helper class for String^ to wchar_t* conversion
	msclr::interop::marshal_context ctx;

	memset(item_info, 0, sizeof(StorageItemInfo));
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), ctx.marshal_as<const wchar_t*>(filePath));

	ItemProperties^ props = fileInfo->Properties;
	if (props != nullptr)
	{
		item_info->Attributes = (DWORD)props->Attributes;
		item_info->Size = props->Length;
		if (props->IsDirectory)
			item_info->Attributes |= FILE_ATTRIBUTE_DIRECTORY;

		if (props->HasTimes)
		{
			try
			{
				item_info->CreationTime = DateTimeToFileTime(props->CreationTimeUtc);
				item_info->ModificationTime = DateTimeToFileTime(props->LastWriteTimeUtc);
			}
			catch (ArgumentOutOfRangeException^)
			{
				//Same as below, invalid date/time values are not crucial
			}
		}
		return GET_ITEM_OK;
	}

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
		// DiscUtils hangs on ext files with holes, so they are read separately
		IO::Stream ^inStr = (fileInfo->ExtReader != nullptr) ? fileInfo->ExtReader->OpenFile(dfi->FullName) : nullptr;
		if (inStr == nullptr)
			inStr = dfi->OpenRead();
		IO::Stream ^outStr = gcnew IO::FileStream(strDestFile, IO::FileMode::Create, IO::FileAccess::Write, IO::FileShare::Read);

		try
		{
			int copyBufSize = 32 * 1024;
			array<Byte> ^copyBuf = gcnew array<Byte>(copyBufSize);

			Int64 bytesLeft = inStr->Length;
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
	if (MODULE_HAS_PREPARE_FILES_EX(LoadParams))
		LoadParams->ApiFuncs.PrepareFilesEx = PrepareFilesEx;

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
