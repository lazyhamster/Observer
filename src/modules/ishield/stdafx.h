// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


// Additional headers
#include <malloc.h>
#include <stdint.h>

#include <vector>
#include <sstream>

#define FREE_NULL(x) if (x) { free(x); x = NULL; }
#define DELETE_NULL(x) if (x) { delete x; x = NULL; }
