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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
void ParseCATPath(const char *pszName, char **ppszCATName, char **ppszFile);
