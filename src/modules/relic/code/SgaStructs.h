#ifndef SgaStructs_h__
#define SgaStructs_h__

#pragma pack(push, 1)

struct file_header_t
{
	char sIdentifier[8];
	unsigned short iVersionMajor;
	unsigned short iVersionMinor;
	char iContentsMD5[16];
	wchar_t sArchiveName[64];
	char iHeaderMD5[16];
	unsigned long iDataHeaderSize;
	unsigned long iDataOffset;
};

struct data_header_t
{
	// All offsets are from the start of this data header
	unsigned long iEntryPointOffset;
	unsigned short int iEntryPointCount;
	unsigned long iDirectoryOffset;
	unsigned short int iDirectoryCount;
	unsigned long iFileOffset;
	unsigned short int iFileCount;
	
	// The string block contains the name of every directory and file in the archive. Note that the string count is
	// the number of strings (i.e. iDirectoryCount + iFileCount), *not* the length (in bytes) of the string block.
	// In version 4.1 SGA files, the string block is encrypted to prevent casual viewing of file names. In this case,
	// placed before the string block is the encryption algorithm ID and key required to decrypt the block.
	unsigned long iStringOffset;
	unsigned short int iStringCount;
};

struct directory_raw_info_t
{
	unsigned long iNameOffset;
	unsigned short int iFirstDirectory;
	unsigned short int iLastDirectory;
	unsigned short int iFirstFile;
	unsigned short int iLastFile;
};

struct file_info_t
{
	unsigned long iNameOffset;
	unsigned long iDataOffset;
	unsigned long iDataLengthCompressed;
	unsigned long iDataLength;
	unsigned long iModificationTime;
	union
	{
		unsigned long iFlags32;
		unsigned short iFlags16;
	};
};

#pragma pack(pop)

#endif // SgaStructs_h__