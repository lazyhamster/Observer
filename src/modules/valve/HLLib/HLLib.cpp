/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your Option) any later
 * version.
 */

#include "stdafx.h"
#include "HLLib.h"

using namespace HLLib;

namespace HLLib
{
	hlBool bInitialized = hlFalse;
	CError LastError = CError();

	PExtractItemStartProc pExtractItemStartProc = 0;
	PExtractItemEndProc pExtractItemEndProc = 0;
	PExtractFileProgressProc pExtractFileProgressProc = 0;
	PValidateFileProgressProc pValidateFileProgressProc = 0;
	PDefragmentProgressProc pDefragmentProgressProc = 0;
	PDefragmentProgressExProc pDefragmentProgressExProc = 0;

	hlBool bOverwriteFiles = hlTrue;
	hlBool bReadEncrypted = hlTrue;
	hlBool bForceDefragment = hlFalse;

	hlVoid hlExtractItemStart(const HLDirectoryItem *pItem)
	{
		if(pExtractItemStartProc != 0)
		{
			pExtractItemStartProc(pItem);
		}
	}

	hlVoid hlExtractItemEnd(const HLDirectoryItem *pItem, hlBool bSuccess)
	{
		if(pExtractItemEndProc != 0)
		{
			pExtractItemEndProc(pItem, bSuccess);
		}
	}

	hlVoid hlExtractFileProgress(const HLDirectoryItem *pFile, hlULongLong uiBytesExtracted, hlULongLong uiBytesTotal, hlBool *pCancel)
	{
		if(pExtractFileProgressProc)
		{
			pExtractFileProgressProc(pFile, static_cast<hlUInt>(uiBytesExtracted), static_cast<hlUInt>(uiBytesTotal), pCancel);
		}
	}

	hlVoid hlValidateFileProgress(const HLDirectoryItem *pFile, hlULongLong uiBytesValidated, hlULongLong uiBytesTotal, hlBool *pCancel)
	{
		if(pValidateFileProgressProc)
		{
			pValidateFileProgressProc(pFile, static_cast<hlUInt>(uiBytesValidated), static_cast<hlUInt>(uiBytesTotal), pCancel);
		}
	}

	hlVoid hlDefragmentProgress(const HLDirectoryItem *pFile, hlUInt uiFilesDefragmented, hlUInt uiFilesTotal, hlULongLong uiBytesDefragmented, hlULongLong uiBytesTotal, hlBool *pCancel)
	{
		if(pDefragmentProgressProc)
		{
			pDefragmentProgressProc(pFile, uiFilesDefragmented, uiFilesTotal, static_cast<hlUInt>(uiBytesDefragmented), static_cast<hlUInt>(uiBytesTotal), pCancel);
		}
		if(pDefragmentProgressExProc)
		{
			pDefragmentProgressExProc(pFile, uiFilesDefragmented, uiFilesTotal, uiBytesDefragmented, uiBytesTotal, pCancel);
		}
	}
}

//
// hlInitialize()
// Initializes all resources.
//
HLLIB_API hlVoid hlInitialize()
{
	if(bInitialized)
	{
		return;
	}

	bInitialized = hlTrue;
	LastError = CError();

	return;
}

//
// hlShutdown()
// Frees all resources.
//
HLLIB_API hlVoid hlShutdown()
{
	if(!bInitialized)
		return;

	bInitialized = hlFalse;
}

HLLIB_API hlBool hlGetBoolean(HLOption eOption)
{
	hlBool bValue = hlFalse;
	hlGetBooleanValidate(eOption, &bValue);
	return bValue;
}

HLLIB_API hlBool hlGetBooleanValidate(HLOption eOption, hlBool *pValue)
{
	switch(eOption)
	{
	case HL_OVERWRITE_FILES:
		*pValue = bOverwriteFiles;
		return hlTrue;
	case HL_READ_ENCRYPTED:
		*pValue = bReadEncrypted;
		return hlTrue;
	case HL_FORCE_DEFRAGMENT:
		*pValue = bForceDefragment;
		return hlTrue;
	}

	return hlFalse;
}

HLLIB_API hlVoid hlSetBoolean(HLOption eOption, hlBool bValue)
{
	switch(eOption)
	{
	case HL_OVERWRITE_FILES:
		bOverwriteFiles = bValue;
		break;
	case HL_READ_ENCRYPTED:
		bReadEncrypted = bValue;
		break;
	case HL_FORCE_DEFRAGMENT:
		bForceDefragment = bValue;
		break;
	}
}

HLLIB_API const hlChar *hlGetString(HLOption eOption)
{
	const hlChar *lpValue = 0;
	hlGetStringValidate(eOption, &lpValue);
	return lpValue ? lpValue : "";
}

HLLIB_API hlBool hlGetStringValidate(HLOption eOption, const hlChar **pValue)
{
	switch(eOption)
	{
	case HL_VERSION:
		*pValue = HL_VERSION_STRING;
		return hlTrue;
	case HL_ERROR:
		*pValue = LastError.GetErrorMessage();
		return hlTrue;
	case HL_ERROR_SYSTEM:
		*pValue = LastError.GetSystemErrorMessage();
		return hlTrue;
	case HL_ERROR_SHORT_FORMATED:
		*pValue = LastError.GetShortFormattedErrorMessage();
		return hlTrue;
	case HL_ERROR_LONG_FORMATED:
		*pValue = LastError.GetLongFormattedErrorMessage();
		return hlTrue;
	default:
		return hlFalse;
	}
}

HLLIB_API hlVoid hlSetString(HLOption eOption, const hlChar *lpValue)
{

}

HLLIB_API const hlVoid *hlGetVoid(HLOption eOption)
{
	const hlVoid *lpValue = 0;
	hlGetVoidValidate(eOption, &lpValue);
	return lpValue;
}

HLLIB_API hlBool hlGetVoidValidate(HLOption eOption, const hlVoid **pValue)
{
	switch(eOption)
	{
	case HL_PROC_EXTRACT_ITEM_START:
		*pValue = (const hlVoid *)pExtractItemStartProc;
		return hlTrue;
	case HL_PROC_EXTRACT_ITEM_END:
		*pValue = (const hlVoid *)pExtractItemEndProc;
		return hlTrue;
	case HL_PROC_EXTRACT_FILE_PROGRESS:
		*pValue = (const hlVoid *)pExtractFileProgressProc;
		return hlTrue;
	case HL_PROC_VALIDATE_FILE_PROGRESS:
		*pValue = (const hlVoid *)pValidateFileProgressProc;
		return hlTrue;
	case HL_PROC_DEFRAGMENT_PROGRESS:
		*pValue = (const hlVoid *)pDefragmentProgressProc;
		return hlTrue;
	case HL_PROC_DEFRAGMENT_PROGRESS_EX:
		*pValue = (const hlVoid *)pDefragmentProgressExProc;
		return hlTrue;
	default:
		return hlFalse;
	}
}

HLLIB_API hlVoid hlSetVoid(HLOption eOption, const hlVoid *pValue)
{
	switch(eOption)
	{
	case HL_PROC_EXTRACT_ITEM_START:
		pExtractItemStartProc = (PExtractItemStartProc)pValue;
		break;
	case HL_PROC_EXTRACT_ITEM_END:
		pExtractItemEndProc = (PExtractItemEndProc)pValue;
		break;
	case HL_PROC_EXTRACT_FILE_PROGRESS:
		pExtractFileProgressProc = (PExtractFileProgressProc)pValue;
		break;
	case HL_PROC_VALIDATE_FILE_PROGRESS:
		pValidateFileProgressProc = (PValidateFileProgressProc)pValue;
		break;
	case HL_PROC_DEFRAGMENT_PROGRESS:
		pDefragmentProgressProc = (PDefragmentProgressProc)pValue;
		break;
	case HL_PROC_DEFRAGMENT_PROGRESS_EX:
		pDefragmentProgressExProc = (PDefragmentProgressExProc)pValue;
		break;
	}
}

HLLIB_API hlBool hlAttributeGetBoolean(HLAttribute *pAttribute)
{
	if(pAttribute->eAttributeType != HL_ATTRIBUTE_BOOLEAN)
	{
		return hlFalse;
	}

	return pAttribute->Value.Boolean.bValue;
}

HLLIB_API hlVoid hlAttributeSetBoolean(HLAttribute *pAttribute, const hlChar *lpName, hlBool bValue)
{
	pAttribute->eAttributeType = HL_ATTRIBUTE_BOOLEAN;
	if(lpName != 0)
	{
		strncpy(pAttribute->lpName, lpName, sizeof(pAttribute->lpName));
		pAttribute->lpName[sizeof(pAttribute->lpName) - 1] = '\0';
	}
	pAttribute->Value.Boolean.bValue = bValue;
}

HLLIB_API hlInt hlAttributeGetInteger(HLAttribute *pAttribute)
{
	if(pAttribute->eAttributeType != HL_ATTRIBUTE_INTEGER)
	{
		return 0;
	}

	return pAttribute->Value.Integer.iValue;
}

HLLIB_API hlVoid hlAttributeSetInteger(HLAttribute *pAttribute, const hlChar *lpName, hlInt iValue)
{
	pAttribute->eAttributeType = HL_ATTRIBUTE_INTEGER;
	if(lpName != 0)
	{
		strncpy(pAttribute->lpName, lpName, sizeof(pAttribute->lpName));
		pAttribute->lpName[sizeof(pAttribute->lpName) - 1] = '\0';
	}
	pAttribute->Value.Integer.iValue = iValue;
}

HLLIB_API hlUInt hlAttributeGetUnsignedInteger(HLAttribute *pAttribute)
{
	if(pAttribute->eAttributeType != HL_ATTRIBUTE_UNSIGNED_INTEGER)
	{
		return 0;
	}

	return pAttribute->Value.UnsignedInteger.uiValue;
}

HLLIB_API hlVoid hlAttributeSetUnsignedInteger(HLAttribute *pAttribute, const hlChar *lpName, hlUInt uiValue, hlBool bHexadecimal)
{
	pAttribute->eAttributeType = HL_ATTRIBUTE_UNSIGNED_INTEGER;
	if(lpName != 0)
	{
		strncpy(pAttribute->lpName, lpName, sizeof(pAttribute->lpName));
		pAttribute->lpName[sizeof(pAttribute->lpName) - 1] = '\0';
	}
	pAttribute->Value.UnsignedInteger.uiValue = uiValue;
	pAttribute->Value.UnsignedInteger.bHexadecimal = bHexadecimal;
}

HLLIB_API hlFloat hlAttributeGetFloat(HLAttribute *pAttribute)
{
	if(pAttribute->eAttributeType != HL_ATTRIBUTE_FLOAT)
	{
		return 0.0f;
	}

	return pAttribute->Value.Float.fValue;
}

HLLIB_API hlVoid hlAttributeSetFloat(HLAttribute *pAttribute, const hlChar *lpName, hlFloat fValue)
{
	pAttribute->eAttributeType = HL_ATTRIBUTE_FLOAT;
	if(lpName != 0)
	{
		strncpy(pAttribute->lpName, lpName, sizeof(pAttribute->lpName));
		pAttribute->lpName[sizeof(pAttribute->lpName) - 1] = '\0';
	}
	pAttribute->Value.Float.fValue = fValue;
}

HLLIB_API const hlChar *hlAttributeGetString(HLAttribute *pAttribute)
{
	if(pAttribute->eAttributeType != HL_ATTRIBUTE_STRING)
	{
		return "";
	}

	return pAttribute->Value.String.lpValue;
}

HLLIB_API hlVoid hlAttributeSetString(HLAttribute *pAttribute, const hlChar *lpName, const hlChar *lpValue)
{
	pAttribute->eAttributeType = HL_ATTRIBUTE_STRING;
	if(lpName != 0)
	{
		strncpy(pAttribute->lpName, lpName, sizeof(pAttribute->lpName));
		pAttribute->lpName[sizeof(pAttribute->lpName) - 1] = '\0';
	}
	if(lpValue != 0)
	{
		strncpy(pAttribute->Value.String.lpValue, lpValue, sizeof(pAttribute->Value.String.lpValue));
		pAttribute->Value.String.lpValue[sizeof(pAttribute->Value.String.lpValue) - 1] = '\0';
	}
	else
	{
		*pAttribute->Value.String.lpValue = '\0';
	}
}
