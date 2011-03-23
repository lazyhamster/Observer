/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your Option) any later
 * version.
 */

#ifndef HLLIB_H
#define HLLIB_H

#include "HLTypes.h"
#include "Error.h"
#include "Package.h"

namespace HLLib
{
	extern hlBool bInitialized;
	extern CError LastError;

	extern PExtractItemStartProc pExtractItemStartProc;
	extern PExtractItemEndProc pExtractItemEndProc;
	extern PExtractFileProgressProc pExtractFileProgressProc;
	extern PValidateFileProgressProc pValidateFileProgressProc;

	hlVoid hlExtractItemStart(const HLDirectoryItem *pItem);
	hlVoid hlExtractItemEnd(const HLDirectoryItem *pItem, hlBool bSuccess);
	hlVoid hlExtractFileProgress(const HLDirectoryItem *pFile, hlULongLong uiBytesExtracted, hlULongLong uiBytesTotal, hlBool *pCancel);
	hlVoid hlValidateFileProgress(const HLDirectoryItem *pFile, hlULongLong uiBytesValidated, hlULongLong uiBytesTotal, hlBool *pCancel);

	extern hlBool bOverwriteFiles;
	extern hlBool bReadEncrypted;
}

#ifdef __cplusplus
extern "C" {
#endif

HLLIB_API hlVoid hlInitialize();
HLLIB_API hlVoid hlShutdown();

//
// Get/Set
//

HLLIB_API const hlVoid *hlGetVoid(HLOption eOption);
HLLIB_API hlBool hlGetVoidValidate(HLOption eOption, const hlVoid **pValue);
HLLIB_API hlVoid hlSetVoid(HLOption eOption, const hlVoid *pValue);

//
// Attributes
//

HLLIB_API hlBool hlAttributeGetBoolean(HLAttribute *pAttribute);
HLLIB_API hlVoid hlAttributeSetBoolean(HLAttribute *pAttribute, const hlChar *lpName, hlBool bValue);

HLLIB_API hlInt hlAttributeGetInteger(HLAttribute *pAttribute);
HLLIB_API hlVoid hlAttributeSetInteger(HLAttribute *pAttribute, const hlChar *lpName, hlInt iValue);

HLLIB_API hlUInt hlAttributeGetUnsignedInteger(HLAttribute *pAttribute);
HLLIB_API hlVoid hlAttributeSetUnsignedInteger(HLAttribute *pAttribute, const hlChar *lpName, hlUInt uiValue, hlBool bHexadecimal);

HLLIB_API hlFloat hlAttributeGetFloat(HLAttribute *pAttribute);
HLLIB_API hlVoid hlAttributeSetFloat(HLAttribute *pAttribute, const hlChar *lpName, hlFloat fValue);

HLLIB_API const hlChar *hlAttributeGetString(HLAttribute *pAttribute);
HLLIB_API hlVoid hlAttributeSetString(HLAttribute *pAttribute, const hlChar *lpName, const hlChar *lpValue);

#ifdef __cplusplus
}
#endif

#endif
