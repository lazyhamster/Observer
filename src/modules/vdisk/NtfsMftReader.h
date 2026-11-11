#pragma once

#include "ModuleDef.h"
#include "ItemProperties.h"

// Builds NTFS files list from MFT records, which are read sequentially.
// Listing through directory indexes needs a disk seek for nearly every directory,
// on a hard disk that makes opening a large volume take minutes, this way it takes seconds.
// Names and parents come from $FILE_NAME attributes, other properties from $STANDARD_INFORMATION
// and $DATA, which are more up to date than the copies in directory indexes.
// Same items are skipped as DiscUtils does: metafiles, DOS names, entries of deleted or reused records.
ref class NtfsMftReader
{
public:
	value struct NameEntry
	{
		int Record;
		int Parent;
		System::UInt16 ParentSequence;
		System::String^ Name;
	};

private:
	System::IO::Stream^ m_volume;
	int m_clusterSize;
	int m_recordSize;
	__int64 m_mftOffset;
	__int64 m_recordCount;

	array<System::Byte>^ m_flags; // RecordFlag* values
	array<System::UInt16>^ m_sequence;
	array<System::UInt32>^ m_attributes;
	array<__int64>^ m_creationTime;
	array<__int64>^ m_writeTime;
	array<__int64>^ m_size;
	System::Collections::Generic::List<NameEntry>^ m_names;

	void ParseRecord(System::Byte* record, int recordIndex);
	void AddItems(int dir, System::String^ prefix, int depth,
		System::Collections::Generic::Dictionary<int, System::Collections::Generic::List<int>^>^ children,
		array<bool>^ visited, System::Collections::Generic::List<ItemProperties^>^ items);
	ItemProperties^ MakeItem(const NameEntry% entry, System::String^ path);

public:
	// Volume is the raw NTFS volume stream
	NtfsMftReader(System::IO::Stream^ volume);

	// Reads all MFT records. Returns false if user aborted listing.
	// Throws if MFT can't be read, files should be listed through directories then.
	bool Scan(const ListProgressCallbacks* progress);

	// Items in tree order, each directory before its content. Name holds path relative to the volume root.
	void GetItems(System::Collections::Generic::List<ItemProperties^>^ items);
};
