#pragma once

#include "ItemProperties.h"
#include "InternalMembers.h"

// Works around DiscUtils ext issues through its internals (the bundled DiscUtils build is fixed).
// If internals are not found, IsAvailable is false and regular DiscUtils API should be used.
// - DiscUtils resolves every symlink while listing a directory, so a single dangling link
//   (e.g. /etc/mtab pointing into /proc) makes the whole directory unreadable,
//   and links to directories show up under the name of their target. Here symlinks are skipped.
// - DiscUtils uses only lower 32 bits of file size.
// - DiscUtils hangs reading files with holes, so files using extents are read with ExtFileStream.
ref class ExtDirectoryReader : public IDirectoryReader
{
private:
	System::Object^ m_fileSystem;
	System::IO::Stream^ m_volume;
	int m_blockSize;
	System::Reflection::MethodInfo^ m_getDirectory;
	System::Reflection::MethodInfo^ m_getDirectoryEntry;
	System::Reflection::MethodInfo^ m_getFile;
	InternalMembers^ m_members;

	System::Object^ GetInode(System::Object^ file);
	__int64 GetFileLength(System::Object^ inode);

public:
	// Volume is the stream DiscUtils was given, it's used to read extent tree blocks
	ExtDirectoryReader(DiscUtils::DiscFileSystem^ fileSystem, System::IO::Stream^ volume);

	property bool IsAvailable { bool get(); }

	virtual void ReadDirectory(System::String^ path, System::Collections::Generic::List<ItemProperties^>^ entries);

	// Returns nullptr if file doesn't use extents, it should be opened with DiscUtils then
	System::IO::Stream^ OpenFile(System::String^ path);
};
