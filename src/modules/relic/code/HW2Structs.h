#ifndef HW2Structs_h__
#define HW2Structs_h__

#pragma pack(push, 1)

// Total size : 180 bytes
struct BIG_ArchHeader
{
	char Magic[8]; // Always "_ARCHIVE"
	uint32_t version;
	char signature1[16]; // MD5 tool signature of archive (MD5 of tool security key and full file data excluding the archive header)
	wchar_t name[64];
	char signature2[16]; // MD5 signature of archive (MD5 of HW2 Root Security Key and archive header data)
	uint32_t sectionHeaderSize;
	uint32_t exactFileDataOffset;
};

// Total size : 6 bytes
struct BIG_SectionRef
{
	uint32_t Offset; // relative to archive header
	uint16_t Count;
};

// Total size : 24 bytes
struct BIG_SectionHeader
{
	BIG_SectionRef TOCList;			// (describes each TOC entry, that is, each folder hierarchy)
	BIG_SectionRef FolderList;		// (describes the folder hierarchy for each TOC)
	BIG_SectionRef FileInfoList;	// (describes each file)
	BIG_SectionRef FileNameList;	// (the list of file names, including folder names)
};

// Total size : 138 bytes
struct BIG_TOCListEntry
{
	char CharacterAliasName[64];
	char CharacterName[64];
	uint16_t FirstFolderIndex;
	uint16_t LastFolderIndex;
	uint16_t FirstFilenameIndex;
	uint16_t LastFilenameIndex;
	uint16_t StartFolderIndexForHierarchy;
};

// Total size : 12 bytes
struct BIG_FolderListEntry
{
	uint32_t FileNameOffset; // (relative to file name list offset)
	uint16_t FirstSubfolderIndex;
	uint16_t LastSubfolderIndex;
	uint16_t FirstFilenameIndex;
	uint16_t LastFilenameIndex;
};

// Total size : 17 bytes
struct BIG_FileInfoListEntry
{
	uint32_t FileNameOffset; // (relative to file name list offset)
	BYTE Flags; // 0x00 if uncompressed
				// 0x10 to decompress during read -- used for large files
				// 0x20 to decompress all at once -- used for small files, like .lua files
	uint32_t FileDataOffset; // (relative to overall file data offset)
	uint32_t CompressedLength;
	uint32_t DecompressedLength;
};

// Total size : 264 bytes
struct BIG_FileHeader
{
	char FileName[256];
	uint32_t FileModificationDate;
	uint32_t UncompressedDataCRC;
};

#pragma pack(pop)

#endif // HW2Structs_h__