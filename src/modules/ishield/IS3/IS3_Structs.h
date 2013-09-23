#ifndef IS3_Structs_h__
#define IS3_Structs_h__

#pragma pack(push, 1)

#define Z_SIG 0x8C655D13

struct CABHEADER
{
	DWORD Signature;	// 0
	BYTE Unk1[8];		// 4
	WORD NumFiles;		// 0c
	BYTE Unk2[4];		// 0e
	DWORD ArcSize;		// 12
	BYTE Unk3[19];		// 16
	DWORD offsEntries;	// 29
	BYTE Unk4[3];		// 2d
	WORD Dirs;			// 31
	BYTE Unk5[204];		// 33
};

#pragma pack(pop)

#endif // IS3_Structs_h__
