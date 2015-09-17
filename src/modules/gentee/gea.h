#ifndef gea_h__
#define gea_h__

#pragma pack(push, 1)

struct geavolume
{
	char name[4];      // GEA\x0
	uint16_t number;    // The number of the volume 0 for the main volume
	uint32_t unique;    // Unique number to detect the volumes
};

// The structure of the first volume
struct geahead
{
	uint8_t  majorver;  // Major version
	uint8_t  minorver;  // Minor version
	FILETIME date;      // System data of the creation
	uint32_t flags;     // Flags
	uint16_t count;     // The count of volumes.
	uint32_t size;      // Header size ( till data )
	int64_t  summary;   // The summary compressed size
	uint32_t infosize;  // The unpacked size of file descriptions
	int64_t  geasize;   // The full size of the main file
	int64_t  volsize;   // The size of the each volume
	int64_t  lastsize;  // The size of the last volume file 
	uint32_t movedsize; // The moved size from the end to the head
	uint8_t  memory;    // Memory for PPMD compression ( * MB )
	uint8_t  blocksize; // The default size of the block ( * 0x40000 KB )
	uint8_t  solidsize; // The previous solid size for LZGE (* 0x40000 KB )
};

struct geacompfile
{
	uint16_t  flags;    // Flags
	FILETIME  ft;       // File time
	uint32_t  size;     // File size
	uint32_t  compsize; // The compressed size
	uint32_t  crc;
};

struct geadata
{
	uint8_t  order;
	uint32_t size;
};

#pragma pack(pop)

#define GEA_MAJOR       1
#define GEA_MINOR       0
#define GEA_MINVOLSIZE  0xFFFF    // The minimum size of the volume
#define GEA_MAXVOLUMES  0xFFFF    // The maximum count of volumes
#define GEA_DESCRESERVE 0x200000  // 2 MB резервировать для записи информации о файлах 
#define GEA_NAME        0x414547  // GEA/x0

// geahead.flags
#define GEAH_PASSWORD   0x0001    // There are protected files
#define GEAH_COMPRESS   0x0002    // File descriptions are compressed

// geafile.flags
#define GEAF_ATTRIB   0x0001    // There is an attribute    
#define GEAF_FOLDER   0x0010    // There is a relative path
#define GEAF_VERSION  0x0020    // There is a version
#define GEAF_GROUP    0x0040    // There is a group id
#define GEAF_PROTECT  0x0080    // There is the number of password
#define GEAF_SOLID    0x0100    // Uses the information of the previous file

// Compression algorithms
#define GEA_STORE  0
#define GEA_LZGE   1
#define GEA_PPMD   2

#endif // gea_h__
