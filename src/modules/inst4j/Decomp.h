#ifndef Decomp_h__
#define Decomp_h__

#include "modulecrt/Streams.h"

bool LzmaDecomp(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize);

#endif // Decomp_h__
