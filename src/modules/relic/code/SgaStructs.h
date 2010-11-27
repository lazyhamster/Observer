#ifndef SgaStructs_h__
#define SgaStructs_h__

#pragma pack(push, 1)

struct _file_header_raw_t
{
	// Formally, these fields are the 'file' header:
	char sIdentifier[8];
	unsigned short iVersionMajor;
	unsigned short iVersionMinor;
	long iContentsMD5[4];
	wchar_t sArchiveName[64];
	long iHeaderMD5[4];
	unsigned long iDataHeaderSize;
	unsigned long iDataOffset;
	unsigned long iPlatform;
	// Formally, these fields are the start of the 'data' header:
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

#pragma pack(pop)

#endif // SgaStructs_h__