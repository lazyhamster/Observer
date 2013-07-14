#ifndef IS6_CabStructs_h__
#define IS6_CabStructs_h__

#pragma pack( push, 1 )

typedef __int64 FILESIZE;

#define CAB_SIG		0x28635349
#define CFG_MAX		0x47

struct CABHEADER
{
	DWORD Signature;			// 0
	DWORD Version;				// 4
	DWORD LinkFlags;			// 8
	DWORD ofsCabDesc;			// c
	DWORD cbCabDesc;			// 10
	FILESIZE ofsCompData;		// 14
	DWORD FirstFile;			// 1c		
	DWORD LastFile;				// 20		
	FILESIZE ofsFirstData;		// 24
	FILESIZE cbFirstExpanded;	// 2c
	FILESIZE cbFirstData;		// 34
	FILESIZE ofsLastData;		// 3c
	FILESIZE cbLastExpanded;	// 44
	FILESIZE cbLastData;		// 4c
	// Array containing password at offs:54 password size limit unknown yet
};	// size=54

#define LINK_NONE	0
#define LINK_PREV	1
#define LINK_NEXT	2
#define LINK_BOTH	3

struct CABDESC
{
	DWORD ofsStrings;				// 0
	DWORD junk11;					// 4
	DWORD ofsCompList;				// 8
	DWORD ofsDFT;					// c
	DWORD junk21;					// 10
	DWORD cbDFT;					// 14
	DWORD cbDFT2;					// 18
	WORD cDirs;						// 1c
	DWORD phpDirs;					// 1e
	WORD Pad01;						// 22
	DWORD cbDirs;					// 24
	DWORD cFiles;					// 28
	DWORD ofsFilesDFT;				// 2c
	WORD cCompTabInfo;				// 30
	DWORD ofsCompTab;				// 32
	DWORD junk51, junk52;			// 36
	DWORD ofsFileGroup[CFG_MAX];	// 3e
	DWORD ofsComponent[CFG_MAX];	// 15a
	DWORD ofsSTypes;				// 276
	DWORD ofsSTab;					// 27a
};	// size=4e

typedef DWORD* DFTABLE;

struct FILEDESC
{
	WORD DescStatus;
	FILESIZE cbExpanded;
	FILESIZE cbCompressed;
	FILESIZE ofsData;
	BYTE MD5Sig[16];
	DWORD dwVerMS;
	DWORD dwVerLS;
	DWORD junk1[2];
	DWORD ofsName;
	WORD DirIndex;
	DWORD Attrs;
	WORD FatDate, FatTime;
	DWORD junk2;
	DWORD PrevDupe;
	DWORD NextDupe;
	BYTE LinkFlags;
	WORD Volume;
};	// size=57

#define FILE_ATTR_READONLY             0x00000001  
#define FILE_ATTR_HIDDEN               0x00000002  
#define FILE_ATTR_SYSTEM               0x00000004  
#define FILE_ATTR_DIRECTORY            0x00000010  
#define FILE_ATTR_ARCHIVE              0x00000020  

#define DESC_SPLIT			1L
#define DESC_ENCRYPTED		2L
#define DESC_COMPRESSED		4L
#define DESC_INVALID		8L

struct COMPFILEENTRY
{
	DWORD ofsName;		// 0
	DWORD ofsDesc;		// 4
	DWORD ofsNext;		// 8, size=C
};

#define COMP_STAT_CRITICAL	0x1
#define COMP_STAT_RECOMMEND	0x2
#define COMP_STAT_STANDARD	0x4

#define COMP_ATTR_LOCKED			0x1L
#define COMP_ATTR_DATA_AS_FILES		0x8L
#define COMP_ATTR_VISIBLE			0x10L
#define COMP_ATTR_ENCRYPTION		0x20L
#define COMP_ATTR_VOLATILE			0x40L

struct COMPONENTDESC
{
	DWORD ofsName;						// 0
	DWORD ofsDescription;				// 4
	DWORD ofsStatusText;				// 8
	WORD Status;						// c
	DWORD ofsPassword;					// e
	DWORD ofsMisc;						// 12
	WORD Index;							// 16
	DWORD ofsDispName;					// 18
	DWORD ofsCDRomFolder;				// 1c
	DWORD ofsHTTPLocation;				// 20
	DWORD ofsFTPLocation;				// 24
	GUID Guid[2];						// 28
	DWORD ofsUnkStr1;					// 48
	BYTE junk41[3];						// 4c
	DWORD Attrs;						// 4f
	DWORD junk51[7];
	WORD cFileGroups;					// 6f
	DWORD ofsFileGroups;				// 71
	WORD cDepends;						// 75
	DWORD ofsDepends;					// 77
	WORD cSubComps;						// 7b
	DWORD ofsSubComps;					// 7d
	DWORD ofsNextComponent;				// 81
	DWORD ofsOnInstalling;				// 85
	DWORD ofsOnInstalled;				// 89
	DWORD ofsOnUninstalling;			// 8d
	DWORD ofsOnUninstalled;				// 91
};// size=95

// Overwrite attributes
struct FG_Flags
{
	DWORD is_ALWAYS_OVERWRITE	:1;//#define FGDESC_ALWAYS_OVERWRITE	0x001L
	DWORD is_NEVER_OVERWRITE	:1;//#define FGDESC_NEVER_OVERWRITE		0x002L
	DWORD pad					:3;
	DWORD is_NEWER_DATE			:1;//#define FGDESC_NEWER_DATE			0x020L
	DWORD pad2					:3;
	DWORD is_NEWER_VERSION		:1;//#define FGDESC_NEWER_VERSION		0x200L
	DWORD pad3					:22;
};

// Group attributes
struct FG_Attrs
{
	WORD	is_Shared				:1;//1#define FGATTR_SHARED			0x01
	WORD	is_Encrypted			:1;//2#define FGATTR_ENCRYPTED		0x02
	WORD	is_Compressed			:1;//3#define FGATTR_COMPRESSED		0x04
	WORD	is_u1					:1;//4
	WORD	is_SelfRegistering		:1;//5#define FGATTR_SELFREGISTER	0x10
	WORD	is_PotentiallyLocked	:1;//6#define FGATTR_LOCKED			0x20
	WORD	is_Uninstall			:1;//7------Active Low-----------
	WORD	is_u2					:9;//8
};

struct FILEGROUPDESC
{
	DWORD		ofsName;
	FILESIZE	cbExpanded;
	FILESIZE	cbCompressed;
	FG_Attrs	Attrs;
	DWORD		FirstFile;
	DWORD		LastFile;
	DWORD		ofsUnkStr1;
	DWORD		ofsOperatingSystem;
	DWORD		ofsLanguage;
	DWORD		ofsUnkStr2;
	DWORD		ofsHTTPLocation;
	DWORD		ofsFTPLocation;
	DWORD		ofsMisc;
	DWORD		ofsTargetDir;
	FG_Flags	OverwriteFlags;
	DWORD		junk1[4];
};

struct SETUPTYPEHEADER
{
	DWORD cSTypes;
	DWORD ofsSTypeTab;
};

struct SETUPTYPEDESC
{
	DWORD	ofsName;
	DWORD	ofsDescription;
	DWORD	ofsDispName;
	SETUPTYPEHEADER	STHeader;
};

#pragma pack( pop )

#endif // IS6_CabStructs_h__
