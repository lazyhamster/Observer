#ifndef Decomp_h__
#define Decomp_h__

#include "modulecrt/Streams.h"

bool Explode(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc);
bool Unstore(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outCrc);
bool LzmaDecomp(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc);
bool Lzma2Decomp(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc);

#endif // Decomp_h__
