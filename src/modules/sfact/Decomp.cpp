#include "stdafx.h"
#include "Decomp.h"

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
	size_t outTotalSize;
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
		return 0;
	}
	return 1;
}

bool Explode(AStream* inStream, size_t inSize, AStream* outStream, size_t *outSize)
{
	LPVOID inBuffer = malloc(inSize);
	if (!inStream->ReadBuffer(inBuffer, inSize)) return false;
	
	InBufPtr inhow = {inBuffer, inSize};
	OutBufPtr outhow = {outStream, 0};
	int blastRet = blast(blast_in_impl, &inhow, blast_out_impl, &outhow);

	free(inBuffer);
	if (outSize) *outSize = outhow.outTotalSize;
	return blastRet == 0;
}
