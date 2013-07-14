#ifndef IS5_CabStructs_h__
#define IS5_CabStructs_h__

#pragma pack(push, 1)

#define CAB_SIG 0x28635349
#define CFG_MAX 0x47

struct CABHEADER
{
	DWORD Signature;		// 0
	DWORD Version;			// 4
	BYTE NextVol;			// 5
	BYTE junk2;				// 6
	WORD junk3;				// 8
	DWORD ofsCabDesc;		// c
	DWORD cbCabDesc;		// 10
	DWORD ofsCompData;		// 14
	DWORD junk1;			// 18
	DWORD FirstFile;		// 1c
	DWORD LastFile;			// 20
	DWORD ofsFirstData;		// 24
	DWORD cbFirstExpanded;	// 28
	DWORD cbFirstHere;		// 2c
	DWORD ofsLastData;		// 30
	DWORD cbLastExpanded;	// 34
	DWORD cbLastHere;		// 38
};	// size=3c

struct CABDESC
{
	DWORD ofsStrings;
	DWORD junk1;
	DWORD ofsCompList;
	DWORD ofsDFT;
	DWORD junk2;
	DWORD cbDFT;
	DWORD cbDFT2;
	DWORD cDirs;
	DWORD junk3[2];
	DWORD cFiles;
	DWORD junk4;
	WORD cCompTabInfo;
	DWORD ofsCompTab;
	DWORD junk5[2];
	DWORD ofsFileGroup[CFG_MAX];
	DWORD ofsComponent[CFG_MAX];
	DWORD ofsSTypes;
	DWORD ofsSTab;
};

typedef DWORD* DFTABLE;

struct FILEDESC
{
	DWORD ofsName;
	DWORD DirIndex;
	WORD DescStatus;
	DWORD cbExpanded;
	DWORD cbCompressed;
	DWORD Attrs;
	DWORD FatDate;
	DWORD FatTime;
	DWORD junk1, junk2;
	DWORD ofsData;
};

#define DESC_NEXT_VOLUME	1L
#define DESC_ENCRYPTED		2L
#define DESC_COMPRESSED		4L
#define DESC_INVALID		8L

struct COMPFILEENTRY
{
	DWORD ofsName;		// 0
	DWORD ofsDesc;		// 4
	DWORD ofsNext;		// 8, size=C
};

struct COMPONENTDESC
{
	DWORD ofsName;				// 0
	DWORD junk1[2];				// 4
	DWORD ofsDescription;		// c
	DWORD ofsStatusText;		// 10
	DWORD junk2[2];				// 14
	DWORD ofsTargetDir;			// 1c
	DWORD junk3;				// 20
	DWORD ofsPassword;			// 24
	DWORD junk4[4];				// 28
	DWORD ofsMisc;				// 38
	DWORD junk5[3];				// 3c
	DWORD ofsDispName;			// 48
	DWORD ofsCDRomFolder;		// 4c
	DWORD ofsHTTPLocation;		// 50
	DWORD ofsFTPLocation;		// 54
	DWORD junk6[6];				// 58
	WORD cFileGroups;			// 70
	DWORD ofsFileGroups;		// 72
	WORD cDepends;				// 76
	DWORD ofsDepends;			// 78
	WORD cSubComps;				// 7c
	DWORD ofsSubComps;			// 7e
	WORD cX3;					// 82
	DWORD ofsX3;				// 84
	DWORD ofsNextComponent;		// 88
};		// size=8c

struct FILEGROUPDESC
{
	DWORD ofsName;
	DWORD cbExpanded;
	DWORD cbCompressed;
	WORD Attr1, Attr2;
	DWORD Shared;
	DWORD SelfRegister;
	DWORD PotentiallyLocked;
	DWORD Compressed;
	DWORD ofsVar1, ofsVar2, ofsVar3;
	DWORD ofsHTTPLocation;
	DWORD ofsFTPLocation;
	DWORD ofsMisc;
	DWORD junk2[5];
	DWORD FirstFile;
	DWORD LastFile;
	WORD FirstVolume;
	WORD LastVolume;
	DWORD ofsVar4;
	BYTE junk3;
};

struct SETUPTYPEHEADER
{
	DWORD cSTypes;
	DWORD ofsSTypeTab;
};

struct SETUPTYPEDESC
{
	DWORD ofsName;
	DWORD ofsDescription;
	DWORD ofsDispName;
	DWORD junk1;
	DWORD ofsSTab;
};

#pragma pack(pop)

#endif // IS5_CabStructs_h__
