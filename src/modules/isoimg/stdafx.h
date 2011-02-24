// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS 1

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


// Additional headers
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(DEBUG) || defined(_DEBUG)
#define DebugString(x) OutputDebugStringA( __STR__(__LINE__)": " ),OutputDebugStringA( x ),OutputDebugStringA( "\n" )
#define __STR2__(x) #x
#define __STR__(x) __STR2__(x)
#else
#define DebugString(x)
#endif
