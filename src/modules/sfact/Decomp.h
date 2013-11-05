#ifndef Decomp_h__
#define Decomp_h__

#include "Streams.h"

bool Explode(AStream* inStream, uint32_t inSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc);

#endif // Decomp_h__
