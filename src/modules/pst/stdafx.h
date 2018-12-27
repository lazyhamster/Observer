// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#define _SCL_SECURE_NO_WARNINGS 1

#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING 1
#define _SILENCE_FPOS_SEEKPOS_DEPRECATION_WARNING 1

// Additional headers
#include <sstream>

#include "pstsdk/pst.h"