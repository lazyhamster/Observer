#ifndef _PACKAGE_STRUCT_H_
#define _PACKAGE_STRUCT_H_

// Table: Directory
struct DirectoryEntry
{
	wchar_t Key[256];			// s72
	wchar_t ParentKey[256];		// S72
	wchar_t DefaultName[512];	// l255
};

// Table: Component
struct ComponentEntry
{
	wchar_t Key[256];			// s72
	wchar_t ID[39];				// S38
	wchar_t Directory[256];		// s72
	INT16 Attributes;			// i2
};

// Table: File
struct FileEntry
{
	wchar_t Key[256];			// s72
	wchar_t Component[256];		// s72
	wchar_t FileName[512];		// l255
	INT32 FileSize;				// i4
	//wchar_t Version[73];		// S72
	//wchar_t Language[21];		// S20
	INT16 Attributes;			// i2
	INT16 Sequence;				// i2
};

// Table: Feature
struct FeatureEntry
{
	wchar_t Key[72];			// s38
	wchar_t Parent[72];			// S38
	wchar_t Title[128];			// L64
	wchar_t Description[512];	// L255
	//UINT16 Display;			// i2
	//UINT16 Level;				// i2
	wchar_t Directory[73];		// S72
};


// Table: Entry
struct MediaEntry
{
	UINT16 DiskId;				// i2
	UINT16 LastSequence;		// i2
	//wchar_t DiskPrompt[65];	// L64
	wchar_t Cabinet[256];		// S255
	//wchar_t VolumeLabel[33];	// S32
	//wchar_t Source[73];		// S72
};

// Table: _Streams
struct StreamEntry
{
	wchar_t Name[63];			// s62
	char* Data;					// V0 (binary stream)
};

// Table: Registry
struct RegistryEntry
{
	wchar_t Key[256];			// s72
	UINT16 Root;				// i2
	wchar_t RegKeyName[256];	// l255
	wchar_t Name[256];			// L255
	wchar_t* Value;				// L0 (variable len)
	wchar_t Component[73];		// s72
};

#endif //_PACKAGE_STRUCT_H_