#ifndef Unpack_h__
#define Unpack_h__

#include "modulecrt/Streams.h"

bool zUnpackData(AStream* inStream, uint32_t packedSize, AStream* outStream, uint32_t *outSize, uint32_t *outCrc32);

#endif // Unpack_h__
