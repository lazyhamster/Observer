// Common/Types.h

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

typedef unsigned char Byte;
typedef short Int16;
typedef unsigned short UInt16;
typedef int Int32;
typedef unsigned int UInt32;
typedef __int64 Int64;
typedef unsigned __int64 UInt64;

#define CHAR_PATH_SEPARATOR '\\'
#define WCHAR_PATH_SEPARATOR L'\\'
#define STRING_PATH_SEPARATOR "\\"
#define WSTRING_PATH_SEPARATOR L"\\"

#if _MSC_VER >= 1300
#define MY_NO_INLINE __declspec(noinline)
#else
#define MY_NO_INLINE
#endif

#define MY_CDECL __cdecl
#define MY_STD_CALL __stdcall
#define MY_FAST_CALL MY_NO_INLINE __fastcall

#ifndef RINOK
#define RINOK(x) { int __result__ = (x); if (__result__) return __result__; }
#endif

#define PURE = 0

#endif