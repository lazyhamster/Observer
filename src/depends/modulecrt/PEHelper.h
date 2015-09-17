#ifndef PEHelper_h__
#define PEHelper_h__

#include "Streams.h"

bool FindFileOverlay(AStream *inStream, int64_t &nOverlayStartOffset, int64_t &nOverlaySize);
bool FindPESection(AStream *inStream, const char* szSectionName, int64_t &nSectionStartOffset, int64_t &nSectionSize);
bool LoadPEResource(const wchar_t* exePath, const wchar_t* rcName, const wchar_t* rcType, AStream *destStream);
std::string GetManifest(const wchar_t* libraryPath);

#endif // PEHelper_h__
