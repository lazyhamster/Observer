#ifndef _ISO_H_
#define _ISO_H_

#pragma pack(push, 1)

typedef struct VolumeDateTime
{
    unsigned char Year;     // Number of years since 1900
    unsigned char Month;    // Month of the year from 1 to 12
    unsigned char Day;      // Day of the Month from 1 to 31
    unsigned char Hour;     // Hour of the day from 0 to 23
    unsigned char Minute;   // Minute of the hour from 0 to 59
    unsigned char Second;   // second of the minute from 0 to 59
             char Zone;     // Offset from Greenwich Mean Time in
                            // number of 15 minute intervals from
                            // -48(West) to +52(East)
} VolumeDateTime;


#define FATTR_HIDDEN       1
#define FATTR_DIRECTORY    2

#define FATTR_ALLOWPARTIAL 32	// This fake flag that uses one of the reserved spots
								// It is needed to make a workaround on hard drive emulation boot files
								// that usually have less physical size then reported one.

typedef struct DirectoryRecord
{
    unsigned char LengthOfDirectoryRecord;  //
    unsigned char ExtendedAttributeRecordLength; // Bytes - this field refers to the
                                            // Extended Attribute Record, which provides
                                            // additional information about a file to
                                            // systems that know how to use it. Since
                                            // few systems use it, we will not discuss
                                            // it here. Refer to ISO 9660:1988 for
                                            // more information.
    __int64       LocationOfExtent;         // This is the Logical Block Number of the
                                            // first Logical Block allocated to the file
    __int64       DataLength;               // Length of the file section in bytes
    VolumeDateTime RecordingDateAndTime;    //
    unsigned char FileFlags;                // One Byte, each bit of which is a Flag:
                                            // bit
                                            // 0 File is Hidden if this bit is 1
                                            // 1 Entry is a Directory if this bit is 1
                                            // 2 Entry is an Associated file is this bit is 1
                                            // 3 Information is structured according to
                                            //   the extended attribute record if this
                                            //   bit is 1
                                            // 4 Owner, group and permissions are
                                            //   specified in the extended attribute
                                            //   record if this bit is 1
                                            // 5 Reserved (0)
                                            // 6 Reserved (0)
                                            // 7 File has more than one directory record
                                            //   if this bit is 1
    unsigned char FileUnitSize;             // This field is only valid if the file is
                                            // recorded in interleave mode
                                            // Otherwise this field is (00)
    unsigned char InterleaveGapSize;        // This field is only valid if the file is
                                            // recorded in interleave mode
                                            // Otherwise this field is (00)
    int VolumeSequenceNumber;               // The ordinal number of the volume in the Volume
                                            // Set on which the file described by the
                                            // directory record is recorded.
    unsigned char LengthOfFileIdentifier;   // Length of File Identifier (LEN_FI)
    unsigned char FileIdentifier[1];
} DirectoryRecord;

typedef struct PathTableRecord
{
    unsigned char LengthOfDirectoryIdentifier;  // Length of Directory Identifier (LEN_DI)
    unsigned char ExtendedAttributeRecordLength;// If an Extended Attribute Record is
                                                // recorded, this is the length in Bytes.
                                                // Otherwise, this is (00)
    unsigned int  LocationOfExtent;             // Logical Block Number of the first Logical
                                                // Block allocated to the Directory
    unsigned short int ParentDirectoryNumber;   // The record number in the Path Table for
                                                // the parent directory of this directory
    unsigned char DirectoryIdentifier[1];       // This field is the same as in the Directory
                                                // Record
} PathTableRecord;

#define XBOX_HIDDEN       0x02
#define XBOX_SYSTEM       0x04
#define XBOX_DIRECTORY    0x10
#define XBOX_ARCHIVE      0x20

typedef struct XBOXDirectoryRecord
{
    short int     LeftSubTree;                  // Offset to left sub-tree entry in DWORDs.
                                                // If this is set to 0, there are no entries
                                                // in this sub-tree.
    short int     RightSubTree;                 // Offset to right sub-tree entry in DWORDs.
                                                // If this is set to 0, there are no entries
                                                // in this sub-tree.
    unsigned int  LocationOfExtent;             // Starting sector of file. Each file may
                                                // consist of multiple contiguous sectors.
    unsigned int  DataLength;                   // Total file size.
    unsigned char FileFlags;                    // File attributes (mostly standard FAT attributes):
                                                //   0x01:    File is read only 
                                                //   0x02:    File is hidden 
                                                //   0x04:    File is system file 
                                                //   0x10:    File is a directory (associated sector(s)
                                                //            will have an XDVDFS directory table
                                                //            stored in them) 
                                                //   0x20:    File is an archive. 
                                                //   0x80:    Normal (i.e. lacking any other attributes) (?)
    unsigned char LengthOfFileIdentifier;       // Length of filename
    unsigned char FileIdentifier[1];
} XBOXDirectoryRecord;

const char CDSignature[] = {'C', 'D', '0', '0', '1'};

typedef struct PrimaryVolumeDescriptor
{
    unsigned char VolumeDescriptorType;
    unsigned char StandardIdentifier[5];        // CD001
    unsigned char VolumeDescriptorVersion;
    unsigned char Ununsed;
    unsigned char SystemIdentifier[32];
    unsigned char VolumeIdentifier[32];
    unsigned char Unused2[8];
    __int64       VolumeSpaceSize;              // Number of logical blocks in the Volume
    unsigned char Unused3[32];
    unsigned int  VolumeSetSize;                // The assigned Volume Set size of the Volume
    unsigned int  VolumeSequenceNumber;         // The ordinal number of the volume in
                                                // the Volume Set
    unsigned int  LogicalBlockSize;             // The size in bytes of a Logical Block
    __int64       PathTableSize;                // Length in bytes of the path table
    unsigned int  LocationOfTypeLPathTable;     // Logical Block Number of first Block allocated
                                                // to the Type L Path Table
    unsigned int  LocationOfOptionalTypeLPathTable; // 0 if Optional Path Table was not recorded,
                                                // otherwise, Logical Block Number of first
                                                // Block allocated to the Optional Type L
                                                // Path Table
    unsigned int  LocationOfTypeMPathTable;     // Logical Block Number of first Block
                                                // allocated to the Type M
    unsigned int  LocationOfOptionalTypeMPathTable; // 0 if Optional Path Table was not
                                                // recorded, otherwise, Logical Path Table,
                                                // Block Number of first Block allocated to the
                                                // Type M Path Table.
    DirectoryRecord DirectoryRecordForRootDirectory; // This is the actual directory record for
                                                // the top of the directory structure
    unsigned char VolumeSetIdentifier[128];     // Name of the multiple volume set of which
                                                // this volume is a member.
    unsigned char PublisherIdentifier[128];     // Identifies who provided the actual data
                                                // contained in the files. acharacters allowed.
    unsigned char DataPreparerIdentifier[128];  // Identifies who performed the actual
                                                // creation of the current volume.
    unsigned char ApplicationIdentifier[128];   // Identifies the specification of how the
                                                // data in the files are recorded.
    unsigned char CopyrightFileIdentifier[37];  // Identifies the file in the root directory
                                                // that contains the copyright notice for
                                                // this volume
    unsigned char AbstractFileIdentifier[37];   // Identifies the file in the root directory
                                                // that contains the abstract statement for
                                                // this volume
    unsigned char BibliographicFileIdentifier[37]; // Identifies the file in the root directory
                                                // that contains bibliographic records.
    unsigned char VolumeCreationDateAndTime[17];// Date and time at which the volume was created
    unsigned char VolumeModificationDateAndTime[17]; // Date and time at which the volume was
                                                // last modified
    unsigned char VolumeExpirationDateAndTime[17]; // Date and Time at which the information in
                                                // the volume may be considered obsolete.
    unsigned char VolumeEffectiveDateAndTime[17];// Date and Time at which the information
                                                // in the volume may be used
    unsigned char FileStructureVersion;         // 1
    unsigned char ReservedForFutureStandardization; // 0
    unsigned char ApplicationUse[512];          // This field is reserved for application use.
                                                // Its content is not specified by ISO-9660.
    unsigned char ReservedForFutureStandardization2[653]; // 0
} PrimaryVolumeDescriptor; // sizeof( PrimaryVolumeDescriptor ) must be 2048

const char XBOXSignature[] = {'M', 'I', 'C', 'R', 'O', 'S', 'O', 'F', 'T', '*',
                              'X', 'B', 'O', 'X', '*', 'M', 'E', 'D', 'I', 'A'};

typedef struct XBOXPrimaryVolumeDescriptor
{
    unsigned char StandardIdentifier[0x14];     // “MICROSOFT*XBOX*MEDIA”
    unsigned int  LocationOfExtent;             // Sector that root directory table resides in
    unsigned int  DataLength;                   // Size of root directory table in bytes
    FILETIME      FileTime;                     // FILETIME structure representing image
                                                // creation time (as in FAT-style date/ FAT-style time).
    unsigned char Unused[0x7c8];                // unused
    unsigned char StandardIdentifier2[0x14];    // “MICROSOFT*XBOX*MEDIA”
} XBOXVolumeDescriptor;

// boot image structures
static const char TORITO[] = "EL TORITO SPECIFICATION";

typedef struct BootRecordVolumeDescriptor
{
    unsigned char BootRecordIndicator;          // Boot Record Indicator, must be 0
    unsigned char StandardIdentifier[5];        // ISO-9660 Identifier, must be "CD001"
    unsigned char VersionOfDescriptor;          // Version of this descriptor, must be 1
    unsigned char BootSystemIdentifier[32];     // Boot System Identifier, must be "EL TORITO SPECIFICATION" padded with 0’s.
    unsigned char Unused[32];                   // Unused, must be 0
    unsigned int  BootCatalogPointer;           // Absolute pointer to first sector of Boot Catalog.
    unsigned char Unused2[1973];                // Unused, must be 0.
} BootRecordVolumeDescriptor;

typedef struct ValidationEntry
{
    unsigned char  HeaderID;                    // Header ID, must be 01
    unsigned char  PlatformID;                  // Platform ID
                                                //   0 = 80x86
                                                //   1=Power PC
                                                //   2=Mac
    unsigned short Reserved;                    // Reserved, must be 0
    unsigned char  ID[24];                      // ID string. This is intended to identify the manufacturer/developer of
                                                //   the CD-ROM.
    short int      Checksum;                    // Checksum Word. This sum of all the words in this record should be 0.
    short int      KeyWord;                     // Key word, must be AA55. This value is included in the checksum.
} ValidationEntry;

typedef struct InitialEntry
{
    unsigned char Bootable;                     // Boot Indicator. 88 = Bootable, 00 = Not Bootable
    unsigned char BootMediaType;                // Boot media type. This specifies what media the boot image is intended to
                                                //   emulate in bits 0-3 as follows, bits 4-7 are reserved and must be 0.
                                                //   Bits 0-3 count as follows:
                                                //   0 - No Emulation
                                                //   1 - 1.2 meg diskette
                                                //   2 - 1.44 meg diskette
                                                //   3 - 2.88 meg diskette
                                                //   4 - Hard Disk (drive 80)
                                                //   5-F - Reserved, invalid at this time
    unsigned short LoadSegment;                 // Load Segment. This is the load segment for the initial boot image. If this
                                                //   value is 0 the system will use the traditional segment of 7C0. If this value
                                                //   is non-zero the system will use the specified segment. This applies to x86
                                                //   architectures only. For "flat" model architectures (such as Motorola) this
                                                //   is the address divided by 10.
    unsigned char SystemType;                   // System Type. This must be a copy of byte 5 (System Type) from the
                                                //   Partition Table found in the boot image.
    unsigned char Reserved;                     // Unused, must be 0
    unsigned short SectorCount;                 // Sector Count. This is the number of virtual/emulated sectors the system
                                                //   will store at Load Segment during the initial boot procedure.
    unsigned int  LoadRBA;                      // Load RBA. This is the start address of the virtual disk. CD’s use
                                                //   Relative/Logical block addressing.
    unsigned char Reserved2[20];                // Unused, must be 0
} InitialEntry;

typedef struct SectionHeaderEntry
{
    unsigned char HeaderIndicator;              // Header Indicator as follows:
                                                //   90 -Header, more headers follow
                                                //   91 - Final Header
    unsigned char PlatformID;                   // Platform ID:
                                                //   0 = 80x86
                                                //   1 = Power PC
                                                //   2 = Mac
    unsigned short NumberOfSectionEntries;      // Number of section entries following this header
    unsigned char ID[28];                       // ID string. This identifies a section. This string will be checked by BIOS and
                                                //   BOOT software. If the string matches, the section should be scanned for boot
                                                //   images.
} SectionHeaderEntry;

typedef struct SectionEntry
{
    unsigned char BootIndicator;                // Boot Indicator. 88 = Bootable, 00 = Not Bootable
    unsigned char BootMediaType;                // Boot media type. This specifies what media the boot image emulates
                                                //   in bits 0-32.
                                                //   Bits 6 and 7 are specific to the type of system.
                                                //   Bits 0-3 count as follows
                                                //   0 No Emulation
                                                //   1 - 1.2 meg diskette
                                                //   2 - 1.44 meg diskette
                                                //   3 - 2.88 meg diskette
                                                //   4 - Hard Disk (drive 80)
                                                //   5-F - Reserved, invalid at this time
                                                //   bit 4 - Reserved, must be 0
                                                //   bit 5 - Continuation Entry Follows
                                                //   bit 6 - Image contains an ATAPI driver
                                                //   bit 7 - Image contains SCSI drivers
    unsigned short LoadSegment;                 // Load Segment. This is the load segment for the initial boot image.
                                                //   If this value is 0 the system will use the traditional segment
                                                //   of 7C0. If this value is non-zero the system will use the specified
                                                //   segment. This applies to x86 architectures only. For "flat" model
                                                //   architectures (such as Motorola) this is the address divided by 10.
    unsigned char SystemType;                   // System Type. This must be a copy of byte 5 (System Type) from the Partition
                                                //   Table found in the boot image.
    unsigned char Reserved;                     // Unused, must be 0
    unsigned short SectorCount;                 // Sector Count. This is the number of virtual/emulated sectors the system will
                                                //   store at Load Segment during the initial boot procedure.
    unsigned int  LoadRBA;                      // Load RBA. This is the start address of the virtual disk. CD’s use
                                                //   Relative/Logical block addressing.
    unsigned char SelectionCriteriaType;        // Selection criteria type. This defines a vendor unique format for bytes 0D-1F.
                                                //   The following formats have currently been assigned:
                                                //   0 - No selection criteria
                                                //   1 - Language and Version Information (IBM)
                                                //   2-FF - Reserved
    unsigned char SelectionCriteria[19];        // Vendor unique selection criteria.
} SectionEntry;

typedef struct SectionEntryExtension
{
    unsigned char ExtensionIndicator;           // Extension Indicator. Must be 44
    unsigned char Bits;                         // Bits 1-4 - Unused
                                                //   5 == 1 - Extension Record follows, 0 - This is final Extension
                                                //   6-7 - Unused
    unsigned char SelectionCriteria[30];        // Vendor unique selection criteria
} SectionEntryExtension;

typedef union CatalogEntry
{
    InitialEntry          Initial;
    SectionHeaderEntry    Header;
    SectionEntry          Entry;
    SectionEntryExtension Extension;
} CatalogEntry;

typedef struct BootCatalog
{
    ValidationEntry Validation;
    CatalogEntry    Entry[2];
} BootCatalog;


typedef struct PrimaryVolumeDescriptorEx
{
    union
    {
        PrimaryVolumeDescriptor     VolumeDescriptor;
        XBOXPrimaryVolumeDescriptor XBOXVolumeDescriptor;
    };
    BootCatalog*             BootCatalog;
    DWORD                    BootImageEntries;
    bool                     Unicode;
    bool                     XBOX;
	bool                     SystemUseAreas;
} PrimaryVolumeDescriptorEx;

typedef struct Directory
{
    wchar_t*     FilePath;
	wchar_t*     FileName;
    PrimaryVolumeDescriptorEx* VolumeDescriptor;
    union
    {
        DirectoryRecord     Record;
        XBOXDirectoryRecord XBOXRecord;
    };
} Directory;

enum IsoImageType
{
	ISOTYPE_RAW,
	ISOTYPE_ISZ
};

typedef struct IsoImage
{
    HANDLE                   hFile;
	IsoImageType             ImageType;
    PrimaryVolumeDescriptorEx* VolumeDescriptors;
    DWORD                    DescriptorNum;
    DWORD                    DataOffset;
    DWORD                    HeaderSize;
    DWORD                    RealBlockSize;
    Directory*               DirectoryList;
    DWORD                    DirectoryCount;
    DWORD                    Index;
} IsoImage;

typedef struct Partition
{
    unsigned char            boot_ind;          /* 0x80 - active */
    unsigned char            head;              /* starting head */
    unsigned char            sector;            /* starting sector */
    unsigned char            cyl;               /* starting cylinder */
    unsigned char            sys_ind;           /* What partition type */
    unsigned char            end_head;          /* end head */
    unsigned char            end_sector;        /* end sector */
    unsigned char            end_cyl;           /* end cylinder */
    unsigned int             start_sect;        /* starting sector counting from 0 */
    unsigned int             nr_sects;          /* nr of sectors in partition */
} Partition;

typedef struct MBR
{
    unsigned char            Loader[0x1be];     // boot loader
    Partition                Partition[4];      // partitions
    unsigned short           Signature;
} MBR;

//////////////////////////////////////////////////////////////////////////

struct SystemUseEntryHeader
{
	char Signature[2];
	unsigned char Length;
	unsigned char Version;
};

#pragma pack(pop)

#endif //_ISO_H_