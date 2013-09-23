#ifndef IS3_Structs_h__
#define IS3_Structs_h__

#pragma pack(push, 1)

#define Z_SIG 0x8C655D13

struct CABHEADER
{
	DWORD Signature;	// 0
	BYTE Unk1[8];		// 4
	WORD cFiles;		// 0c
	BYTE Unk2[4];		// 0e
	DWORD ArchiveSize;	// 12
	DWORD UnpackedSize;	// 16
	BYTE Unk3[15];		// 1A
	DWORD ofsDirList;	// 29
	DWORD cbDirList;	// 2d
	WORD cDirs;			// 31
	DWORD ofsFileList;	// 35
	DWORD cbFileList;	// 39
};

struct DIRDESC
{
	WORD cFiles;
	WORD cbDescSize;
	WORD cbNameLength;
};

struct FILEDESC
{
	BYTE Unk1[3];
	DWORD cbExpanded;
	DWORD cbCompressed;
	DWORD ofsCompData;
	WORD FatDate;
	WORD FatTime;
	DWORD Attrs;
	WORD cbDescSize;
	BYTE Unk2[4];
	BYTE cbNameLength;
};

//TODO: figure out where is dir index in FILEDESC

#pragma pack(pop)

#endif // IS3_Structs_h__
