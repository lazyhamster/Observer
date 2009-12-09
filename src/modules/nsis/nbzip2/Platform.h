#ifndef ___PLATFORM__H___
#define ___PLATFORM__H___

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>

#ifndef NSISCALL
#  define NSISCALL __stdcall
#endif

#ifndef NSIS_COMPRESS_BZIP2_LEVEL
  #define NSIS_COMPRESS_BZIP2_LEVEL 9
#endif

#define EXEHEAD

#endif
