#pragma once

// Item properties read together with the directory, so they don't have to be looked up by path later
ref class ItemProperties
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

// Lists a directory together with properties of its items
interface class IDirectoryReader
{
	// Path is relative to the volume root, without leading backslash, empty for the root itself
	void ReadDirectory(System::String^ path, System::Collections::Generic::List<ItemProperties^>^ entries);
};
