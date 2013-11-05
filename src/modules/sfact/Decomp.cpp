#include "stdafx.h"
#include "Decomp.h"
#include "zlib/zlib.h"

extern "C"
{
#include "blast/blast.h"
}

struct InBufPtr
{
	void* buf;
	unsigned int bufSize;
};

struct OutBufPtr
{
	AStream* strm;
	uint32_t outTotalSize;
	uint32_t outCrc;
};

static unsigned int blast_in_impl(void *how, unsigned char **buf)
{
	InBufPtr* ptr = (InBufPtr*)how;
	*buf = (unsigned char *)ptr->buf;
	return ptr->bufSize;
}

static int blast_out_impl(void *how, unsigned char *buf, unsigned len)
{
	OutBufPtr* ptr = (OutBufPtr*) how;
	if (ptr->strm->WriteBuffer(buf, len))
	{
		ptr->outTotalSize += len;
		ptr->outCrc = crc32(ptr->outCrc, buf, len);
		return 0;
	}
	return 1;
}

bool Explode(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc)
{
	LPVOID inBuffer = malloc(inSize);
	if (!inStream->ReadBuffer(inBuffer, inSize)) return false;
	
	InBufPtr inhow = {inBuffer, inSize};
	OutBufPtr outhow = {outStream, 0, 0};
	int blastRet = blast(blast_in_impl, &inhow, blast_out_impl, &outhow);

	free(inBuffer);
	if (outSize) *outSize = outhow.outTotalSize;
	if (outCrc) *outCrc = outhow.outCrc;
	return blastRet == 0;
}

bool Unstore(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outCrc)
{
	const uint32_t bufferSize = 32 * 1024;
	uint8_t buffer[bufferSize];

	uint32_t bytesLeft = inSize;
	uint32_t crcVal = 0;
	while (bytesLeft > 0)
	{
		uint32_t copySize = min(bytesLeft, bufferSize);
		if (!inStream->ReadBuffer(buffer, copySize) || !outStream->WriteBuffer(buffer, copySize))
			return false;

		bytesLeft -= copySize;
		crcVal = crc32(crcVal, buffer, copySize);
	}
	
	if (outCrc) *outCrc = crcVal;
	return true;
}
