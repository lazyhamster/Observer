/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your Option) any later
 * version.
 */

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include "HLTypes.h"
#include "Error.h"

namespace HLLib
{
	hlULong Adler32(const hlByte *lpBuffer, hlUInt uiBufferSize, hlULong uiAdler32 = 0);
	hlULong CRC32(const hlByte *lpBuffer, hlUInt uiBufferSize, hlULong uiCRC = 0);
}

#endif
