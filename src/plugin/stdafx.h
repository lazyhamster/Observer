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
#include <stdint.h>
#include <wchar.h>
#include <Shlwapi.h>

// STL
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
//using namespace std;

#define ARRAY_SIZE(x) ( sizeof(x) / sizeof(x[0]) )
