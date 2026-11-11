#include "stdafx.h"
#include "NtfsDirectoryReader.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Reflection;
using namespace DiscUtils;

NtfsDirectoryReader::NtfsDirectoryReader(Ntfs::NtfsFileSystem^ fileSystem)
{
	m_members = gcnew InternalMembers();
	try
	{
		// DiscUtils takes file length from directory entry only if this option is set (it is by default)
		if (!fileSystem->NtfsOptions->FileLengthFromDirectoryEntries) return;

		Assembly^ assembly = Ntfs::NtfsFileSystem::typeid->Assembly;
		Type^ referenceType = assembly->GetType("DiscUtils.Ntfs.FileRecordReference");
		Type^ directoryType = assembly->GetType("DiscUtils.Ntfs.Directory");
		Type^ transactionType = assembly->GetType("DiscUtils.Ntfs.NtfsTransaction");
		if (referenceType == nullptr || directoryType == nullptr || transactionType == nullptr) return;

		Type^ fsType = Ntfs::NtfsFileSystem::typeid;
		MethodInfo^ getDirectoryEntry = InternalMembers::FindMethod(fsType, "GetDirectoryEntry", String::typeid);
		MethodInfo^ getDirectory = InternalMembers::FindMethod(fsType, "GetDirectory", referenceType);

		array<Type^>^ boolArg = { Boolean::typeid };
		MethodInfo^ getAllEntries = directoryType->GetMethod("GetAllEntries", BindingFlags::Instance | BindingFlags::Public, nullptr, boolArg, nullptr);

		if (getDirectoryEntry == nullptr || getDirectory == nullptr || getAllEntries == nullptr) return;

		m_fileSystem = fileSystem;
		m_getDirectoryEntry = getDirectoryEntry;
		m_getDirectory = getDirectory;
		m_getAllEntries = getAllEntries;
		m_transactionType = transactionType;
	}
	catch (Exception^)
	{
		// Leave reader unavailable
	}
}

bool NtfsDirectoryReader::IsAvailable::get()
{
	return m_fileSystem != nullptr;
}

void NtfsDirectoryReader::ReadDirectory(String^ path, List<ItemProperties^>^ entries)
{
	// Same steps as NtfsFileSystem.DoSearch, which is used by GetDirectories and GetFiles
	IDisposable^ transaction = (IDisposable^) Activator::CreateInstance(m_transactionType, true);
	try
	{
		Object^ dirEntry = InternalMembers::Invoke(m_getDirectoryEntry, m_fileSystem, path);
		if (dirEntry == nullptr)
			throw gcnew DirectoryNotFoundException("The directory was not found: " + path);

		Object^ dir = InternalMembers::Invoke(m_getDirectory, m_fileSystem, m_members->Get(dirEntry, "Reference"));
		if (dir == nullptr)
			throw gcnew DirectoryNotFoundException("The directory was not found: " + path);

		// Filtered entries, options for hidden and system files are applied there
		Collections::IEnumerable^ dirEntries = dynamic_cast<Collections::IEnumerable^>(InternalMembers::Invoke(m_getAllEntries, dir, true));
		if (dirEntries == nullptr)
			throw gcnew InvalidOperationException("Unable to read directory entries: " + path);

		for each (Object^ entry in dirEntries)
		{
			Object^ details = m_members->Get(entry, "Details");

			ItemProperties^ info = gcnew ItemProperties();
			info->Name = (String^) m_members->Get(details, "FileName");
			if (String::IsNullOrEmpty(info->Name)) continue;

			info->Attributes = (FileAttributes) m_members->Get(details, "FileAttributes");
			info->IsDirectory = (info->Attributes & FileAttributes::Directory) != FileAttributes(0);
			info->Length = info->IsDirectory ? 0 : (__int64) Convert::ToUInt64(m_members->Get(details, "RealSize"));
			info->CreationTimeUtc = (DateTime) m_members->Get(details, "CreationTime");
			info->LastWriteTimeUtc = (DateTime) m_members->Get(details, "ModificationTime");
			info->HasTimes = true;

			entries->Add(info);
		}
	}
	finally
	{
		delete transaction;
	}
}
