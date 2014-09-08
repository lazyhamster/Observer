#ifndef PEHelper_h__
#define PEHelper_h__

#include "Streams.h"

bool FindFileOverlay(AStream *inStream, int64_t &nOverlayStartOffset, int64_t &nOverlaySize);
std::string GetManifest(const wchar_t* libraryPath);

#endif // PEHelper_h__
