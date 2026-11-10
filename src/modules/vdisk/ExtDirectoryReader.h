#pragma once

// Item properties read together with the directory, so they don't have to be looked up by path later
ref class ExtEntryInfo
{
public:
	System::String^ Name;
	bool IsDirectory;
	System::IO::FileAttributes Attributes;
	__int64 Length;
	bool HasTimes;
	System::DateTime CreationTimeUtc;
	System::DateTime LastWriteTimeUtc;
};

// Works around DiscUtils ext issues through its internals (the bundled DiscUtils build is fixed).
// If internals are not found, IsAvailable is false and regular DiscUtils API should be used.
// - DiscUtils resolves every symlink while listing a directory, so a single dangling link
//   (e.g. /etc/mtab pointing into /proc) makes the whole directory unreadable,
//   and links to directories show up under the name of their target. Here symlinks are skipped.
// - DiscUtils uses only lower 32 bits of file size.
// - DiscUtils hangs reading files with holes, so files using extents are read with ExtFileStream.
ref class ExtDirectoryReader
{
private:
	System::Object^ m_fileSystem;
	System::IO::Stream^ m_volume;
	int m_blockSize;
	System::Reflection::MethodInfo^ m_getDirectory;
	System::Reflection::MethodInfo^ m_getDirectoryEntry;
	System::Reflection::MethodInfo^ m_getFile;
	System::Collections::Generic::Dictionary<System::String^, System::Reflection::MemberInfo^>^ m_members;

	System::Object^ Invoke(System::Reflection::MethodInfo^ method, System::Object^ arg);
	System::Object^ GetMember(System::Object^ obj, System::String^ name);
	System::Object^ GetInode(System::Object^ file);
	__int64 GetFileLength(System::Object^ inode);

public:
	// Volume is the stream DiscUtils was given, it's used to read extent tree blocks
	ExtDirectoryReader(DiscUtils::DiscFileSystem^ fileSystem, System::IO::Stream^ volume);

	property bool IsAvailable { bool get(); }

	// Path is relative to the volume root, without leading backslash, empty for the root itself
	void ReadDirectory(System::String^ path, System::Collections::Generic::List<ExtEntryInfo^>^ entries);

	// Returns nullptr if file doesn't use extents, it should be opened with DiscUtils then
	System::IO::Stream^ OpenFile(System::String^ path);
};
