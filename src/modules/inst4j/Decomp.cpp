#include "stdafx.h"
#include "Decomp.h"

#include "7zip/C/LzmaDec.h"
#include "7zip/C/Alloc.h"

bool LzmaDecomp(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize)
{
	int64_t decompSize = 0;
	unsigned char props[LZMA_PROPS_SIZE];
	unsigned char *inbuf, *outbuf;

	size_t inlen = inSize - LZMA_PROPS_SIZE - sizeof(decompSize);
	size_t outlen;

	inStream->ReadBuffer(props, sizeof(props));
	inStream->ReadBuffer(&decompSize, sizeof(decompSize));

	outlen = (size_t)decompSize;
	inbuf = (unsigned char*)malloc(inlen);
	outbuf = (unsigned char*)malloc(outlen);

	inStream->ReadBuffer(inbuf, inlen);

	ELzmaStatus status;
	SRes res = LzmaDecode(outbuf, &outlen, inbuf, &inlen, props, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);

	if (res == SZ_OK)
	{
		outStream->WriteBuffer(outbuf, outlen);

		if (outSize != nullptr)
		{
			*outSize = (uint32_t)outlen;
		}
	}

	free(inbuf);
	free(outbuf);

	return (res == SZ_OK);
}
