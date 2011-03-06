/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your Option) any later
 * version.
 */

#ifndef UTILITY_H
#define UTILITY_H

#include "HLTypes.h"
#include "Error.h"

namespace HLLib
{
	hlBool GetFileExists(const hlChar *lpPath);
	hlBool GetFolderExists(const hlChar *lpPath);

	hlBool GetFileSize(const hlChar *lpPath, hlUInt &uiFileSize);

	hlBool CreateFolder(const hlChar *lpPath);

	hlVoid FixupIllegalCharacters(hlChar *lpName);
	hlVoid RemoveIllegalCharacters(hlChar *lpName);
}

#endif
