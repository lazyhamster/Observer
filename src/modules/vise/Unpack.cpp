#include "stdafx.h"
#include "Unpack.h"
#include "zlib.h"

#define BSIZE 32*1024

static void deobfuscate(uint8_t* buf, uint32_t bufSize)
{
	for (uint32_t i = 0; i < bufSize; i+=2)
	{
		uint8_t tmp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = tmp;
	}
}

bool zUnpackData(AStream* inStream, uint32_t packedSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc32)
{
	int ret;
	unsigned char buf_in[BSIZE] = {0};
	unsigned char buf_out[BSIZE] = {0};
	unsigned int have;

	uint32_t unpackedSize = 0;
	unsigned long crc = crc32(0L, Z_NULL, 0);;
	
	z_stream strm = {0};

	ret = inflateInit2(&strm, -MAX_WBITS);
	if (ret != Z_OK) return false;

	uint32_t bytesLeft = packedSize;
	while ((bytesLeft > 0) && (ret != Z_STREAM_END) && (ret >= 0))
	{
		uint32_t readSize = min(bytesLeft, BSIZE);
		if (!inStream->ReadBuffer(buf_in, readSize))
		{
			ret = Z_ERRNO;
			break;
		}

		deobfuscate(buf_in, readSize);

		strm.next_in = buf_in;
		strm.avail_in = readSize;

		// Run inflate() on input until output buffer not full
		do
		{
			strm.avail_out = BSIZE;
			strm.next_out = buf_out;

			ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_NEED_DICT)
				ret = Z_DATA_ERROR;
			if (ret < 0) break;

			have = BSIZE - strm.avail_out;
			if (have == 0) break;

			// Write extracted data to output file
			if (outStream != nullptr && !outStream->WriteBuffer(buf_out, have))
			{
				ret = Z_ERRNO;
				break;
			}
			
			crc = crc32(crc, buf_out, have);
			unpackedSize += have;

		} while (strm.avail_out == 0 && ret != Z_STREAM_END);
	}

	inflateEnd(&strm);

	if (ret == Z_STREAM_END)
	{
		if (outSize)
			*outSize = unpackedSize;
		if (outCrc32)
			*outCrc32 = crc;

		return true;
	}

/*
	int ret;
	unsigned int have;
	unsigned char buf_in[BSIZE] = {0};
	unsigned char buf_out[BSIZE] = {0};
	DWORD dwRead;

	z_stream strm = {0};
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	ret = inflateInit2(&strm, -15);
	if (ret != Z_OK) return false;

	// Remember position in input file	
	LARGE_INTEGER nInPos = {0};
	nInPos.LowPart = SetFilePointer(inFile, 0, &nInPos.HighPart, FILE_CURRENT);

	info.crc = crc32(0L, Z_NULL, 0);
	info.packedSize = 0;
	info.unpackedSize = 0;

	// Decompress until deflate stream ends or end of file
	do
	{
		if (!ReadFile(inFile, buf_in, BSIZE, &dwRead, NULL) || (dwRead == 0))
		{
			ret = Z_ERRNO;
			break;
		}

		strm.next_in = buf_in;
		strm.avail_in = dwRead;
		// Run inflate() on input until output buffer not full
		do
		{
			strm.avail_out = BSIZE;
			strm.next_out = buf_out;

			ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_NEED_DICT)
				ret = Z_DATA_ERROR;
			if (ret < 0) break;

			have = BSIZE - strm.avail_out;
			if (have == 0) break;

			// Write extracted data to output file
			if (writer != NULL && !writer->SaveData(buf_out, have))
			{
				ret = Z_ERRNO;
				break;
			}
			info.crc = crc32(info.crc, buf_out, have);
			info.unpackedSize += have;

		} while (strm.avail_out == 0 && ret != Z_STREAM_END);

		info.packedSize += (dwRead - strm.avail_in);

		// Done when inflate() says it's done (or error)
	} while (ret != Z_STREAM_END && (ret >= 0 || ret == Z_BUF_ERROR));

	inflateEnd(&strm);

	// Position file pointer at the end of packed data
	if (ret == Z_STREAM_END)
	{
		nInPos.QuadPart += info.packedSize;
		SetFilePointer(inFile, nInPos.LowPart, &nInPos.HighPart, FILE_BEGIN);
		return true;
	}
*/

	return false;
}
