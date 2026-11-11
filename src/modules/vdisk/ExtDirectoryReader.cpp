#include "stdafx.h"
#include "ExtDirectoryReader.h"
#include "ExtFileStream.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Reflection;
using namespace DiscUtils;

static const UInt32 InodeFlagExtents = 0x80000; // EXT4_EXTENTS_FL
static const int MaxExtentDepth = 8;

ExtDirectoryReader::ExtDirectoryReader(DiscFileSystem^ fileSystem, Stream^ volume)
	: m_volume(volume)
{
	m_members = gcnew InternalMembers();
	try
	{
		BindingFlags flags = BindingFlags::Instance | BindingFlags::NonPublic;

		// ExtFileSystem is a facade, the real file system is kept in a private field
		FieldInfo^ wrappedField = Vfs::VfsFileSystemFacade::typeid->GetField("_wrapped", flags);
		if (wrappedField == nullptr) return;

		Object^ realFileSystem = wrappedField->GetValue(fileSystem);
		if (realFileSystem == nullptr) return;

		MethodInfo^ getDirectory = nullptr;
		MethodInfo^ getDirectoryEntry = nullptr;
		MethodInfo^ getFile = nullptr;
		for each (MethodInfo^ method in realFileSystem->GetType()->GetMethods(flags))
		{
			array<ParameterInfo^>^ params = method->GetParameters();
			if (params->Length != 1) continue;

			Type^ argType = params[0]->ParameterType;
			if (method->Name == "GetDirectory" && argType == String::typeid)
				getDirectory = method;
			else if (method->Name == "GetDirectoryEntry" && argType == String::typeid)
				getDirectoryEntry = method;
			else if (method->Name == "GetFile" && Vfs::VfsDirEntry::typeid->IsAssignableFrom(argType))
				getFile = method; // There is also GetFile(string)
		}
		if (getDirectory == nullptr || getDirectoryEntry == nullptr || getFile == nullptr) return;

		// s_log_block_size in the primary superblock
		array<Byte>^ value = gcnew array<Byte>(4);
		volume->Position = 1024 + 0x18;
		if (volume->Read(value, 0, 4) != 4) return;
		UInt32 logBlockSize = BitConverter::ToUInt32(value, 0);
		if (logBlockSize > 6) return; // 64K is the largest block size
		m_blockSize = 1024 << logBlockSize;

		m_fileSystem = realFileSystem;
		m_getDirectory = getDirectory;
		m_getDirectoryEntry = getDirectoryEntry;
		m_getFile = getFile;
	}
	catch (Exception^)
	{
		// Leave reader unavailable
	}
}

bool ExtDirectoryReader::IsAvailable::get()
{
	return m_fileSystem != nullptr;
}

Object^ ExtDirectoryReader::GetInode(Object^ file)
{
	return m_members->Get(file, "Inode");
}

__int64 ExtDirectoryReader::GetFileLength(Object^ inode)
{
	// i_size_high is called i_dir_acl in old ext2 definitions, DiscUtils uses that name and ignores it
	UInt64 low = Convert::ToUInt64(m_members->Get(inode, "FileSize"));
	UInt64 high = Convert::ToUInt64(m_members->Get(inode, "DirAcl"));
	return (__int64)((high << 32) | low);
}

void ExtDirectoryReader::ReadDirectory(String^ path, List<ItemProperties^>^ entries)
{
	Object^ dir = InternalMembers::Invoke(m_getDirectory, m_fileSystem, path);

	Collections::IEnumerable^ dirEntries = dynamic_cast<Collections::IEnumerable^>(m_members->Get(dir, "AllEntries"));
	if (dirEntries == nullptr)
		throw gcnew InvalidOperationException("Unable to read directory entries: " + path);

	for each (Object^ item in dirEntries)
	{
		Vfs::VfsDirEntry^ entry = dynamic_cast<Vfs::VfsDirEntry^>(item);
		if (entry == nullptr || entry->IsSymlink) continue;

		String^ name = entry->FileName;
		if (String::IsNullOrEmpty(name) || name == "." || name == "..") continue;

		ItemProperties^ info = gcnew ItemProperties();
		info->Name = name;
		info->IsDirectory = entry->IsDirectory;

		try
		{
			Object^ fileObj = InternalMembers::Invoke(m_getFile, m_fileSystem, entry);
			Vfs::IVfsFile^ file = dynamic_cast<Vfs::IVfsFile^>(fileObj);
			if (file == nullptr) continue;

			info->Attributes = file->FileAttributes;
			info->Length = info->IsDirectory ? 0 : GetFileLength(GetInode(fileObj));
			try
			{
				info->CreationTimeUtc = file->CreationTimeUtc;
				info->LastWriteTimeUtc = file->LastWriteTimeUtc;
				info->HasTimes = true;
			}
			catch (ArgumentOutOfRangeException^)
			{
				// Invalid time values are not a reason to hide the item
			}
		}
		catch (Exception^)
		{
			// Unreadable inode, skip the item
			continue;
		}

		entries->Add(info);
	}
}

Stream^ ExtDirectoryReader::OpenFile(String^ path)
{
	Object^ entry = InternalMembers::Invoke(m_getDirectoryEntry, m_fileSystem, path);
	if (entry == nullptr)
		throw gcnew FileNotFoundException("File not found", path);

	Object^ inode = GetInode(InternalMembers::Invoke(m_getFile, m_fileSystem, entry));
	if ((Convert::ToUInt32(m_members->Get(inode, "Flags")) & InodeFlagExtents) == 0)
		return nullptr;

	// Root node of extent tree is stored in the inode, DiscUtils has already parsed it
	Object^ root = m_members->Get(inode, "Extents");
	if (root == nullptr)
		return nullptr;

	List<ExtFileStream::Run>^ runs = gcnew List<ExtFileStream::Run>();
	Collections::IEnumerable^ leaves = dynamic_cast<Collections::IEnumerable^>(m_members->Get(root, "Extents"));
	Collections::IEnumerable^ indexes = dynamic_cast<Collections::IEnumerable^>(m_members->Get(root, "Index"));

	if (leaves != nullptr)
	{
		for each (Object^ ext in leaves)
		{
			UInt32 len = Convert::ToUInt32(m_members->Get(ext, "NumBlocks"));
			ExtFileStream::Run run;
			run.LogicalBlock = Convert::ToUInt32(m_members->Get(ext, "FirstLogicalBlock"));
			run.Initialized = (len <= 32768); // Longer length means unwritten extent
			run.BlockCount = run.Initialized ? len : len - 32768;
			run.PhysicalBlock = (Convert::ToUInt64(m_members->Get(ext, "FirstPhysicalBlockHi")) << 32) | Convert::ToUInt64(m_members->Get(ext, "FirstPhysicalBlockLow"));
			runs->Add(run);
		}
	}
	else if (indexes != nullptr)
	{
		for each (Object^ idx in indexes)
		{
			UInt64 leaf = (Convert::ToUInt64(m_members->Get(idx, "LeafPhysicalBlockHi")) << 32) | Convert::ToUInt64(m_members->Get(idx, "LeafPhysicalBlockLo"));
			ExtFileStream::ReadExtentNode(m_volume, m_blockSize, leaf, MaxExtentDepth, runs);
		}
	}

	return gcnew ExtFileStream(m_volume, m_blockSize, GetFileLength(inode), runs);
}
