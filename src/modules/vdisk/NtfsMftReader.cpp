#include "stdafx.h"
#include "NtfsMftReader.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;

namespace
{
	const int RootRecord = 5;
	const int FirstUserRecord = 24; // DiscUtils hides records below this as metafiles
	const int FixupStride = 512; // Update sequence covers each 512 bytes, whatever the sector size is
	const int MaxDepth = 256;

	const Byte RecordFlagInUse = 0x01;
	const Byte RecordFlagDirectory = 0x02;
	const Byte RecordFlagHasStdInfo = 0x04;

	const UInt32 AttrStandardInformation = 0x10;
	const UInt32 AttrFileName = 0x30;
	const UInt32 AttrData = 0x80;
	const UInt32 AttrEnd = 0xFFFFFFFF;

	const Byte NamespaceDos = 2;

	inline UInt16 U16(const Byte* p) { return *(const UInt16*)p; }
	inline UInt32 U32(const Byte* p) { return *(const UInt32*)p; }
	inline __int64 I64(const Byte* p) { return *(const __int64*)p; }

	// Applies update sequence, returns false if record is damaged
	bool ApplyFixup(Byte* record, int size)
	{
		int usaOffset = U16(record + 4);
		int usaCount = U16(record + 6);
		if (usaCount < 1 || usaOffset + usaCount * 2 > size || (usaCount - 1) * FixupStride > size)
			return false;

		UInt16 usn = U16(record + usaOffset);
		for (int i = 1; i < usaCount; i++)
		{
			Byte* tail = record + i * FixupStride - 2;
			if (U16(tail) != usn) return false;
			*(UInt16*)tail = U16(record + usaOffset + i * 2);
		}
		return true;
	}
}

ref struct DataRun
{
	__int64 Lcn;
	__int64 Clusters;
};

NtfsMftReader::NtfsMftReader(Stream^ volume)
	: m_volume(volume)
{
}

static array<Byte>^ ReadExact(Stream^ stream, __int64 offset, int count)
{
	array<Byte>^ data = gcnew array<Byte>(count);
	stream->Position = offset;
	int done = 0;
	while (done < count)
	{
		int n = stream->Read(data, done, count - done);
		if (n <= 0) throw gcnew EndOfStreamException("Unexpected end of NTFS volume");
		done += n;
	}
	return data;
}

bool NtfsMftReader::Scan(const ListProgressCallbacks* progress)
{
	// Boot sector
	array<Byte>^ boot = ReadExact(m_volume, 0, 512);
	if (Text::Encoding::ASCII->GetString(boot, 3, 8) != "NTFS    ")
		throw gcnew InvalidDataException("Not an NTFS boot sector");

	int bytesPerSector = BitConverter::ToUInt16(boot, 0x0B);
	int sectorsPerCluster = boot[0x0D];
	if (sectorsPerCluster > 0x80) sectorsPerCluster = 1 << (256 - sectorsPerCluster);
	m_clusterSize = bytesPerSector * sectorsPerCluster;

	int recordValue = (SByte)boot[0x40];
	m_recordSize = (recordValue < 0) ? (1 << -recordValue) : recordValue * m_clusterSize;
	if (m_clusterSize <= 0 || m_recordSize < 1024 || m_recordSize > 65536)
		throw gcnew InvalidDataException("Unsupported NTFS geometry");

	m_mftOffset = BitConverter::ToInt64(boot, 0x30) * m_clusterSize;

	// $MFT record tells where the rest of MFT is
	array<Byte>^ mftRecord = ReadExact(m_volume, m_mftOffset, m_recordSize);
	List<DataRun^>^ runs = gcnew List<DataRun^>();
	__int64 mftSize = 0;
	{
		pin_ptr<Byte> pinned = &mftRecord[0];
		Byte* r = pinned;
		if (U32(r) != 0x454C4946 || !ApplyFixup(r, m_recordSize))
			throw gcnew InvalidDataException("Invalid $MFT record");

		int used = Math::Min((int)U32(r + 0x18), m_recordSize);
		for (int off = U16(r + 0x14); off + 16 <= used; )
		{
			UInt32 type = U32(r + off);
			int len = (int)U32(r + off + 4);
			if (type == AttrEnd || len < 16 || off + len > used) break;

			if (type == AttrData && r[off + 9] == 0 && r[off + 8] != 0)
			{
				// Without attribute list all runs are here, otherwise use directory listing
				if (I64(r + off + 0x10) != 0)
					throw gcnew NotSupportedException("$MFT data is split between records");
				mftSize = I64(r + off + 0x30);
				__int64 lastVcn = I64(r + off + 0x18);

				__int64 lcn = 0, vcns = 0;
				for (int p = off + U16(r + off + 0x20); p < off + len && r[p] != 0; )
				{
					int lenBytes = r[p] & 0x0F, ofsBytes = r[p] >> 4;
					if (lenBytes == 0 || lenBytes > 8 || ofsBytes > 8 || p + 1 + lenBytes + ofsBytes > off + len)
						throw gcnew InvalidDataException("Invalid $MFT data runs");

					__int64 clusters = 0;
					for (int i = lenBytes - 1; i >= 0; i--) clusters = (clusters << 8) | r[p + 1 + i];

					if (ofsBytes == 0)
						throw gcnew InvalidDataException("Sparse $MFT data");
					__int64 delta = (SByte)r[p + lenBytes + ofsBytes]; // Signed, sign comes from the highest byte
					for (int i = ofsBytes - 2; i >= 0; i--) delta = (delta << 8) | r[p + 1 + lenBytes + i];
					lcn += delta;

					DataRun^ run = gcnew DataRun();
					run->Lcn = lcn;
					run->Clusters = clusters;
					runs->Add(run);
					vcns += clusters;
					p += 1 + lenBytes + ofsBytes;
				}

				if (vcns != lastVcn + 1)
					throw gcnew NotSupportedException("$MFT data runs are incomplete");
				break;
			}
			off += len;
		}
	}
	if (runs->Count == 0 || mftSize <= 0)
		throw gcnew InvalidDataException("$MFT data not found");

	m_recordCount = mftSize / m_recordSize;
	if (m_recordCount <= RootRecord || m_recordCount > Int32::MaxValue)
		throw gcnew InvalidDataException("Invalid MFT size");

	int count = (int)m_recordCount;
	m_flags = gcnew array<Byte>(count);
	m_sequence = gcnew array<UInt16>(count);
	m_attributes = gcnew array<UInt32>(count);
	m_creationTime = gcnew array<__int64>(count);
	m_writeTime = gcnew array<__int64>(count);
	m_size = gcnew array<__int64>(count);
	m_names = gcnew List<NameEntry>();

	// Read runs in large chunks, record can cross chunk and run boundaries
	const int ChunkSize = 4 * 1024 * 1024;
	array<Byte>^ buffer = gcnew array<Byte>(ChunkSize + m_recordSize);
	int filled = 0;
	int recordIndex = 0;
	__int64 mftLeft = mftSize;

	for each (DataRun^ run in runs)
	{
		__int64 runOffset = run->Lcn * m_clusterSize;
		__int64 runLeft = Math::Min(run->Clusters * m_clusterSize, mftLeft);
		mftLeft -= runLeft;

		while (runLeft > 0 && recordIndex < count)
		{
			int toRead = (int)Math::Min((__int64)(ChunkSize - filled), runLeft);
			m_volume->Position = runOffset;
			int got = 0;
			while (got < toRead)
			{
				int n = m_volume->Read(buffer, filled + got, toRead - got);
				if (n <= 0) throw gcnew EndOfStreamException("Unexpected end of MFT");
				got += n;
			}
			runOffset += toRead;
			runLeft -= toRead;
			filled += toRead;

			int processed = 0;
			{
				pin_ptr<Byte> pinned = &buffer[0];
				while (filled - processed >= m_recordSize && recordIndex < count)
				{
					ParseRecord(pinned + processed, recordIndex);
					processed += m_recordSize;
					recordIndex++;
				}
			}

			// Keep a partial record for the next read
			if (processed > 0 && processed < filled)
				Array::Copy(buffer, processed, buffer, 0, filled - processed);
			filled -= processed;

			if (progress && progress->ListProgress)
			{
				// Names found so far is close to the number of items, records give the estimate
				if (!progress->ListProgress(progress->signalContext, m_names->Count, m_recordCount - FirstUserRecord))
					return false;
			}
		}
	}

	if ((m_flags[RootRecord] & (RecordFlagInUse | RecordFlagDirectory)) != (RecordFlagInUse | RecordFlagDirectory))
		throw gcnew InvalidDataException("Root directory record not found");

	return true;
}

void NtfsMftReader::ParseRecord(Byte* r, int recordIndex)
{
	if (U32(r) != 0x454C4946) return; // "FILE"
	if (!ApplyFixup(r, m_recordSize)) return;

	UInt16 flags = U16(r + 0x16);
	__int64 baseRef = I64(r + 0x20) & 0x0000FFFFFFFFFFFF;
	bool isBase = (baseRef == 0);
	if (!(flags & 0x0001)) return; // Not in use

	// Extension records hold more attributes of their base record
	int owner = isBase ? recordIndex : (int)baseRef;
	if (owner < 0 || owner >= m_recordCount) return;

	if (isBase)
	{
		m_sequence[owner] = U16(r + 0x10);
		m_flags[owner] |= RecordFlagInUse;
		if (flags & 0x0002) m_flags[owner] |= RecordFlagDirectory;
	}

	int used = Math::Min((int)U32(r + 0x18), m_recordSize);
	for (int off = U16(r + 0x14); off + 16 <= used; )
	{
		UInt32 type = U32(r + off);
		int len = (int)U32(r + off + 4);
		if (type == AttrEnd || len < 16 || off + len > used) break;

		bool resident = (r[off + 8] == 0);
		int nameLength = r[off + 9];
		const Byte* value = nullptr;
		int valueLength = 0;
		if (resident && len >= 0x18)
		{
			valueLength = (int)U32(r + off + 0x10);
			int valueOffset = U16(r + off + 0x14);
			if (valueOffset + valueLength <= len)
				value = r + off + valueOffset;
		}

		if (type == AttrStandardInformation && value && valueLength >= 0x24)
		{
			m_creationTime[owner] = I64(value + 0x00);
			m_writeTime[owner] = I64(value + 0x08);
			m_attributes[owner] = U32(value + 0x20);
			m_flags[owner] |= RecordFlagHasStdInfo;
		}
		else if (type == AttrFileName && value && valueLength >= 0x42)
		{
			int nameChars = value[0x40];
			Byte nameSpace = value[0x41];
			if (nameSpace != NamespaceDos && nameChars > 0 && 0x42 + nameChars * 2 <= valueLength)
			{
				__int64 parentRef = I64(value);
				NameEntry entry;
				entry.Record = owner;
				entry.Parent = (int)(parentRef & 0x0000FFFFFFFFFFFF);
				entry.ParentSequence = (UInt16)(parentRef >> 48);
				entry.Name = gcnew String((wchar_t*)(value + 0x42), 0, nameChars);
				m_names->Add(entry);
			}

			if (!(m_flags[owner] & RecordFlagHasStdInfo))
			{
				// Used only if $STANDARD_INFORMATION is missing
				m_creationTime[owner] = I64(value + 0x08);
				m_writeTime[owner] = I64(value + 0x10);
				m_attributes[owner] = U32(value + 0x38);
			}
		}
		else if (type == AttrData && nameLength == 0)
		{
			if (resident)
				m_size[owner] = valueLength;
			else if (len >= 0x38 && I64(r + off + 0x10) == 0) // Sizes are valid in the first extent only
				m_size[owner] = I64(r + off + 0x30);
		}

		off += len;
	}
}

ItemProperties^ NtfsMftReader::MakeItem(const NameEntry% entry, String^ path)
{
	int rec = entry.Record;
	ItemProperties^ item = gcnew ItemProperties();
	item->Name = path;
	item->IsDirectory = (m_flags[rec] & RecordFlagDirectory) != 0;

	// Same conversion as DiscUtils does for directory entries
	FileAttributes attr = (FileAttributes)(m_attributes[rec] & 0xFFFF);
	if (item->IsDirectory) attr = attr | FileAttributes::Directory;
	item->Attributes = attr;

	item->Length = item->IsDirectory ? 0 : m_size[rec];
	try
	{
		item->CreationTimeUtc = DateTime::FromFileTimeUtc(m_creationTime[rec]);
		item->LastWriteTimeUtc = DateTime::FromFileTimeUtc(m_writeTime[rec]);
		item->HasTimes = true;
	}
	catch (ArgumentOutOfRangeException^)
	{
		// Invalid time values are not a reason to hide the item
	}
	return item;
}

ref class NameComparer : IComparer<int>
{
	List<NtfsMftReader::NameEntry>^ m_names;
public:
	NameComparer(List<NtfsMftReader::NameEntry>^ names) : m_names(names) {}
	virtual int Compare(int a, int b)
	{
		return String::Compare(m_names[a].Name, m_names[b].Name, StringComparison::OrdinalIgnoreCase);
	}
};

void NtfsMftReader::AddItems(int dir, String^ prefix, int depth, Dictionary<int, List<int>^>^ children, array<bool>^ visited, List<ItemProperties^>^ items)
{
	List<int>^ entries;
	if (depth > MaxDepth || !children->TryGetValue(dir, entries)) return;

	// Directories first, then files, same as other listings
	for each (int i in entries)
	{
		NameEntry entry = m_names[i];
		if (!(m_flags[entry.Record] & RecordFlagDirectory)) continue;

		String^ path = prefix + entry.Name;
		items->Add(MakeItem(entry, path));

		// Directories can't have hard links, this is protection from corrupted data
		if (!visited[entry.Record])
		{
			visited[entry.Record] = true;
			AddItems(entry.Record, path + "\\", depth + 1, children, visited, items);
		}
	}

	for each (int i in entries)
	{
		NameEntry entry = m_names[i];
		if (m_flags[entry.Record] & RecordFlagDirectory) continue;
		items->Add(MakeItem(entry, prefix + entry.Name));
	}
}

void NtfsMftReader::GetItems(List<ItemProperties^>^ items)
{
	// Names of records in use, in directories which are still the same records the names point to
	Dictionary<int, List<int>^>^ children = gcnew Dictionary<int, List<int>^>();
	for (int i = 0; i < m_names->Count; i++)
	{
		NameEntry entry = m_names[i];
		if (entry.Record < FirstUserRecord) continue;
		if (!(m_flags[entry.Record] & RecordFlagInUse)) continue;
		if (entry.Parent < 0 || entry.Parent >= m_recordCount) continue;

		Byte parentFlags = m_flags[entry.Parent];
		if ((parentFlags & (RecordFlagInUse | RecordFlagDirectory)) != (RecordFlagInUse | RecordFlagDirectory)) continue;
		if (m_sequence[entry.Parent] != entry.ParentSequence) continue;

		List<int>^ list;
		if (!children->TryGetValue(entry.Parent, list))
		{
			list = gcnew List<int>();
			children[entry.Parent] = list;
		}
		list->Add(i);
	}

	NameComparer^ comparer = gcnew NameComparer(m_names);
	for each (List<int>^ list in children->Values)
		list->Sort(comparer);

	array<bool>^ visited = gcnew array<bool>((int)m_recordCount);
	visited[RootRecord] = true;
	AddItems(RootRecord, String::Empty, 0, children, visited, items);
}
