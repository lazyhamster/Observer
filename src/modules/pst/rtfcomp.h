#ifndef rtfcomp_h__
#define rtfcomp_h__

// header for compressed stream
typedef struct _lzfuhdr
{
	DWORD cbSize;		// total number of bytes following this field
	DWORD cbRawSize;	// size before compression
	DWORD dwMagic;		// identifies this as a compressed stream
	DWORD dwCRC;		// CRC-32 of the compressed data for error checking
} LZFUHDR;

// magic number that identifies the stream as a compressed stream
#define dwMagicCompressedRTF 0x75465a4c

// magic number that identifies the stream as a uncompressed stream
#define dwMagicUncompressedRTF 0x414c454d

bool lzfu_decompress(uint8_t* compressedData, size_t compressedDataSize, uint8_t* uncompressedData, size_t uncompressedDataSize);
DWORD calculateWeakCrc32(uint8_t* buf, DWORD bufSize);

#endif // rtfcomp_h__
