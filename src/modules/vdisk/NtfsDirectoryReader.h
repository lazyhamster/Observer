#pragma once

#include "ItemProperties.h"
#include "InternalMembers.h"

// DiscUtils looks up NTFS item properties by path, every property separately, so reading them
// for a large volume takes longer than listing it. All of them come from the directory index entry,
// so here they are taken from entries while listing the directory, through DiscUtils internals.
// If internals are not found, IsAvailable is false and regular DiscUtils API should be used.
ref class NtfsDirectoryReader : public IDirectoryReader
{
private:
	DiscUtils::Ntfs::NtfsFileSystem^ m_fileSystem;
	System::Reflection::MethodInfo^ m_getDirectoryEntry;
	System::Reflection::MethodInfo^ m_getDirectory;
	System::Reflection::MethodInfo^ m_getAllEntries;
	System::Type^ m_transactionType;
	InternalMembers^ m_members;

public:
	NtfsDirectoryReader(DiscUtils::Ntfs::NtfsFileSystem^ fileSystem);

	property bool IsAvailable { bool get(); }

	virtual void ReadDirectory(System::String^ path, System::Collections::Generic::List<ItemProperties^>^ entries);
};
