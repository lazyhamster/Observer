#include "stdafx.h"
#include "FatLongNameReader.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Text;

ref class FatLongNameReader
{
private:
	literal int DirEntrySize = 32;
	literal int MaxDirSize = 65536 * DirEntrySize; // Limit from FAT specification
	literal int MaxDirDepth = 128;
	literal int MaxLongNameLength = 255;
	literal int LongNameCharsPerEntry = 13;
	literal int FatCacheSize = 64 * 1024;

	static initonly array<int>^ LongNameCharOffsets = gcnew array<int> { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };

	IO::Stream^ m_volume;
	Encoding^ m_shortNameEncoding;
	Dictionary<String^, String^>^ m_longNames;
	Dictionary<UInt32, bool>^ m_visitedDirs; // Protection from loops on corrupted volumes

	int m_fatBits;
	UInt32 m_clusterCount;
	int m_clusterSize;
	Int64 m_fatOffset;
	Int64 m_rootDirOffset;
	int m_rootDirSize;
	UInt32 m_rootCluster;
	Int64 m_dataOffset;

	array<Byte>^ m_fatCache;
	Int64 m_fatCacheOffset;
	int m_fatCacheBytes;

	FatLongNameReader(IO::Stream^ volume, Encoding^ shortNameEncoding)
		: m_volume(volume), m_shortNameEncoding(shortNameEncoding)
	{
		m_longNames = gcnew Dictionary<String^, String^>();
		m_visitedDirs = gcnew Dictionary<UInt32, bool>();
		m_fatCache = gcnew array<Byte>(FatCacheSize);
	}

	int ReadVolume(Int64 offset, array<Byte>^ buffer, int bufferOffset, int count)
	{
		m_volume->Position = offset;

		int total = 0;
		while (total < count)
		{
			int bytesRead = m_volume->Read(buffer, bufferOffset + total, count - total);
			if (bytesRead <= 0) break;
			total += bytesRead;
		}
		return total;
	}

	bool ReadBootSector()
	{
		array<Byte>^ boot = gcnew array<Byte>(512);
		if (ReadVolume(0, boot, 0, boot->Length) != boot->Length) return false;

		int bytesPerSector = BitConverter::ToUInt16(boot, 11);
		int sectorsPerCluster = boot[13];
		int reservedSectors = BitConverter::ToUInt16(boot, 14);
		int numFats = boot[16];
		int rootEntries = BitConverter::ToUInt16(boot, 17);
		UInt32 totalSectors = BitConverter::ToUInt16(boot, 19);
		UInt32 sectorsPerFat = BitConverter::ToUInt16(boot, 22);

		if (totalSectors == 0) totalSectors = BitConverter::ToUInt32(boot, 32);
		if (sectorsPerFat == 0) sectorsPerFat = BitConverter::ToUInt32(boot, 36);

		bool validSectorSize = bytesPerSector >= 512 && bytesPerSector <= 4096 && (bytesPerSector & (bytesPerSector - 1)) == 0;
		if (!validSectorSize || sectorsPerCluster == 0 || numFats == 0 || sectorsPerFat == 0) return false;

		m_rootDirSize = rootEntries * DirEntrySize;
		UInt32 rootDirSectors = (UInt32)((m_rootDirSize + bytesPerSector - 1) / bytesPerSector);
		UInt64 firstDataSector = (UInt64)reservedSectors + (UInt64)numFats * sectorsPerFat + rootDirSectors;
		if (firstDataSector >= totalSectors) return false;

		m_clusterSize = sectorsPerCluster * bytesPerSector;
		m_clusterCount = (UInt32)((totalSectors - firstDataSector) / sectorsPerCluster);
		m_fatBits = (m_clusterCount < 4085) ? 12 : ((m_clusterCount < 65525) ? 16 : 32);

		m_fatOffset = (Int64)reservedSectors * bytesPerSector;
		m_rootDirOffset = m_fatOffset + (Int64)numFats * sectorsPerFat * bytesPerSector;
		m_dataOffset = (Int64)firstDataSector * bytesPerSector;
		m_rootCluster = BitConverter::ToUInt32(boot, 44);

		return true;
	}

	bool IsDataCluster(UInt32 cluster)
	{
		return cluster >= 2 && (UInt64)cluster < (UInt64)m_clusterCount + 2;
	}

	UInt32 GetNextCluster(UInt32 cluster)
	{
		Int64 entryOffset = (m_fatBits == 12) ? (Int64)cluster + cluster / 2 : (Int64)cluster * (m_fatBits / 8);
		Int64 offset = m_fatOffset + entryOffset;

		if (offset < m_fatCacheOffset || offset + (Int64)sizeof(UInt32) > m_fatCacheOffset + m_fatCacheBytes)
		{
			m_fatCacheOffset = offset;
			m_fatCacheBytes = ReadVolume(offset, m_fatCache, 0, FatCacheSize);
			if (m_fatCacheBytes < (int)sizeof(UInt32)) return 0;
		}

		UInt32 value = BitConverter::ToUInt32(m_fatCache, (int)(offset - m_fatCacheOffset));
		switch (m_fatBits)
		{
		case 12:
			return (cluster & 1) ? ((value >> 4) & 0xFFF) : (value & 0xFFF);
		case 16:
			return value & 0xFFFF;
		default:
			return value & 0x0FFFFFFF;
		}
	}

	static Byte GetShortNameChecksum(array<Byte>^ data, int entryPos)
	{
		Byte sum = 0;
		for (int i = 0; i < 11; i++)
			sum = (Byte)(((sum & 1) << 7) + (sum >> 1) + data[entryPos + i]);
		return sum;
	}

	// Name goes to the plugin as a path element, so it should not be able to change the path structure
	static bool IsSafeLongName(String^ name)
	{
		if (name->Length == 0 || name->Length > MaxLongNameLength) return false;
		if (name->IndexOfAny(IO::Path::GetInvalidFileNameChars()) >= 0) return false;

		Char lastChar = name[name->Length - 1];
		return lastChar != ' ' && lastChar != '.';
	}

	static String^ MakeLongName(array<Char>^ chars)
	{
		int length = Array::IndexOf(chars, (Char)0);
		String^ name = gcnew String(chars, 0, (length >= 0) ? length : chars->Length);
		return IsSafeLongName(name) ? name : nullptr;
	}

	// Should give the same result as FullName elements in DiscUtils
	String^ GetShortName(array<Byte>^ data, int entryPos)
	{
		String^ name = m_shortNameEncoding->GetString(data, entryPos, 8)->TrimEnd(L' ');
		String^ ext = m_shortNameEncoding->GetString(data, entryPos + 8, 3)->TrimEnd(L' ');
		return (ext->Length > 0) ? name + "." + ext : name;
	}

	void ParseDirectory(array<Byte>^ data, String^ parentPath, int depth)
	{
		array<Char>^ longName = nullptr;
		int nextOrdinal = 0;
		Byte longNameChecksum = 0;

		for (int pos = 0; pos + DirEntrySize <= data->Length; pos += DirEntrySize)
		{
			Byte firstByte = data[pos];
			if (firstByte == 0) break;

			if (firstByte == 0xE5)
			{
				longName = nullptr;
				continue;
			}

			Byte attributes = data[pos + 11];
			if ((attributes & 0x3F) == 0x0F)
			{
				int ordinal = firstByte & 0x1F;
				if ((firstByte & 0x40) && ordinal > 0)
				{
					longName = gcnew array<Char>(ordinal * LongNameCharsPerEntry);
					nextOrdinal = ordinal;
					longNameChecksum = data[pos + 13];
				}

				if (longName == nullptr || ordinal != nextOrdinal || data[pos + 13] != longNameChecksum)
				{
					longName = nullptr;
					continue;
				}

				for (int i = 0; i < LongNameCharsPerEntry; i++)
					longName[(ordinal - 1) * LongNameCharsPerEntry + i] = BitConverter::ToChar(data, pos + LongNameCharOffsets[i]);

				nextOrdinal--;
				continue;
			}

			bool hasLongName = longName != nullptr && nextOrdinal == 0 && longNameChecksum == GetShortNameChecksum(data, pos);
			String^ entryLongName = hasLongName ? MakeLongName(longName) : nullptr;
			longName = nullptr;

			if ((attributes & 0x08) || firstByte == '.') continue; // Skip volume label and links to self/parent

			String^ shortName = GetShortName(data, pos);
			String^ shortPath = (parentPath->Length > 0) ? parentPath + "\\" + shortName : shortName;

			if (entryLongName != nullptr)
				m_longNames[shortPath] = entryLongName;

			if ((attributes & 0x10) && depth < MaxDirDepth)
			{
				UInt32 firstCluster = BitConverter::ToUInt16(data, pos + 26);
				if (m_fatBits == 32) firstCluster |= (UInt32)BitConverter::ToUInt16(data, pos + 20) << 16;

				ScanClusterDirectory(firstCluster, shortPath, depth + 1);
			}
		}
	}

	void ScanClusterDirectory(UInt32 firstCluster, String^ path, int depth)
	{
		if (!IsDataCluster(firstCluster) || m_visitedDirs->ContainsKey(firstCluster)) return;
		m_visitedDirs[firstCluster] = true;

		List<UInt32>^ clusters = gcnew List<UInt32>();
		int maxClusters = MaxDirSize / m_clusterSize + 1;
		for (UInt32 cluster = firstCluster; IsDataCluster(cluster) && clusters->Count < maxClusters; cluster = GetNextCluster(cluster))
			clusters->Add(cluster);

		array<Byte>^ data = gcnew array<Byte>(clusters->Count * m_clusterSize);
		for (int i = 0; i < clusters->Count; i++)
		{
			Int64 clusterOffset = m_dataOffset + (Int64)(clusters[i] - 2) * m_clusterSize;
			if (ReadVolume(clusterOffset, data, i * m_clusterSize, m_clusterSize) != m_clusterSize) break;
		}

		ParseDirectory(data, path, depth);
	}

	void ScanRootDirectory()
	{
		if (m_fatBits == 32)
		{
			ScanClusterDirectory(m_rootCluster, String::Empty, 0);
		}
		else
		{
			array<Byte>^ data = gcnew array<Byte>(m_rootDirSize);
			ReadVolume(m_rootDirOffset, data, 0, data->Length);
			ParseDirectory(data, String::Empty, 0);
		}
	}

public:
	static Dictionary<String^, String^>^ ReadLongNames(IO::Stream^ volume, Encoding^ shortNameEncoding)
	{
		FatLongNameReader^ reader = gcnew FatLongNameReader(volume, shortNameEncoding);
		try
		{
			if (reader->ReadBootSector())
				reader->ScanRootDirectory();
		}
		catch (Exception^)
		{
			//Long names are optional, so we continue with everything found so far
		}
		return reader->m_longNames;
	}
};

Dictionary<String^, String^>^ ReadFatLongNames(IO::Stream^ volume, Encoding^ shortNameEncoding)
{
	return FatLongNameReader::ReadLongNames(volume, shortNameEncoding);
}
