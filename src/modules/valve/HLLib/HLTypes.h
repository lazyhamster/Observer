/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef HLTYPE_H
#define HLTYPE_H

#define HLLIB_API

typedef unsigned char		hlBool;
typedef char				hlChar;
typedef unsigned char		hlByte;
typedef signed short		hlShort;
typedef unsigned short		hlUShort;
typedef signed int			hlInt;
typedef unsigned int		hlUInt;
typedef signed long			hlLong;
typedef unsigned long		hlULong;
typedef signed long long	hlLongLong;
typedef unsigned long long	hlULongLong;
typedef float				hlSingle;
typedef double				hlDouble;
typedef void				hlVoid;
typedef wchar_t				hlWChar;

#ifdef _MSC_VER
	typedef unsigned __int8		hlUInt8;
	typedef unsigned __int16	hlUInt16;
	typedef unsigned __int32	hlUInt32;
	typedef unsigned __int64	hlUInt64;
#else
#	include <stdint.h>

	typedef uint8_t		hlUInt8;
	typedef uint16_t	hlUInt16;
	typedef uint32_t	hlUInt32;
	typedef uint64_t	hlUInt64;
#endif

typedef hlSingle		hlFloat;

#define hlFalse			0
#define hlTrue			1

#define HL_VERSION_NUMBER	((2 << 24) | (4 << 16) | (5 << 8) | 0)
#define HL_VERSION_STRING	"2.4.5"

#define HL_ID_INVALID 0xffffffff

#define HL_DEFAULT_PACKAGE_TEST_BUFFER_SIZE 8
#define HL_DEFAULT_VIEW_SIZE 131072
#define HL_DEFAULT_COPY_BUFFER_SIZE 131072

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	HL_VERSION = 0,
	HL_ERROR,
	HL_ERROR_SYSTEM,
	HL_ERROR_SHORT_FORMATED,
	HL_ERROR_LONG_FORMATED,
	HL_PROC_OPEN,
	HL_PROC_CLOSE,
	HL_PROC_READ,
	HL_PROC_WRITE,
	HL_PROC_SEEK,
	HL_PROC_TELL,
	HL_PROC_SIZE,
	HL_PROC_EXTRACT_ITEM_START,
	HL_PROC_EXTRACT_ITEM_END,
	HL_PROC_EXTRACT_FILE_PROGRESS,
	HL_PROC_VALIDATE_FILE_PROGRESS,
	HL_OVERWRITE_FILES,
	HL_PACKAGE_BOUND,
	HL_PACKAGE_ID,
	HL_PACKAGE_SIZE,
	HL_PACKAGE_TOTAL_ALLOCATIONS,
	HL_PACKAGE_TOTAL_MEMORY_ALLOCATED,
	HL_PACKAGE_TOTAL_MEMORY_USED,
	HL_READ_ENCRYPTED,
	HL_FORCE_DEFRAGMENT,
	HL_PROC_DEFRAGMENT_PROGRESS,
	HL_PROC_DEFRAGMENT_PROGRESS_EX,
	HL_PROC_SEEK_EX,
	HL_PROC_TELL_EX,
	HL_PROC_SIZE_EX
} HLOption;

typedef enum
{
	HL_MODE_INVALID = 0x00,
	HL_MODE_READ = 0x01,
	HL_MODE_WRITE = 0x02,
	HL_MODE_CREATE = 0x04,
	HL_MODE_VOLATILE = 0x08,
	HL_MODE_NO_FILEMAPPING = 0x10,
	HL_MODE_QUICK_FILEMAPPING = 0x20
} HLFileMode;

typedef enum
{
	HL_SEEK_BEGINNING = 0,
	HL_SEEK_CURRENT,
	HL_SEEK_END
} HLSeekMode;

typedef enum
{
	HL_ITEM_NONE = 0,
	HL_ITEM_FOLDER,
	HL_ITEM_FILE
} HLDirectoryItemType;

typedef enum
{
	HL_ORDER_ASCENDING = 0,
	HL_ORDER_DESCENDING
} HLSortOrder;

typedef enum
{
	HL_FIELD_NAME = 0,
	HL_FIELD_SIZE
} HLSortField;

typedef enum
{
	HL_FIND_FILES = 0x01,
	HL_FIND_FOLDERS = 0x02,
	HL_FIND_NO_RECURSE = 0x04,
	HL_FIND_CASE_SENSITIVE = 0x08,
	HL_FIND_MODE_STRING = 0x10,
	HL_FIND_MODE_SUBSTRING = 0x20,
	HL_FIND_MODE_WILDCARD = 0x00,
	HL_FIND_ALL = HL_FIND_FILES | HL_FIND_FOLDERS
} HLFindType;

typedef enum
{
	HL_STREAM_NONE = 0,
	HL_STREAM_FILE,
	HL_STREAM_GCF,
	HL_STREAM_MAPPING,
	HL_STREAM_MEMORY,
	HL_STREAM_PROC,
	HL_STREAM_NULL
} HLStreamType;

typedef enum
{
	HL_MAPPING_NONE = 0,
	HL_MAPPING_FILE,
	HL_MAPPING_MEMORY,
	HL_MAPPING_STREAM
} HLMappingType;

typedef enum
{
	HL_PACKAGE_NONE = 0,
	HL_PACKAGE_BSP,
	HL_PACKAGE_GCF,
	HL_PACKAGE_PAK,
	HL_PACKAGE_VBSP,
	HL_PACKAGE_WAD,
	HL_PACKAGE_XZP,
	HL_PACKAGE_ZIP,
	HL_PACKAGE_NCF,
	HL_PACKAGE_VPK,
	HL_PACKAGE_SGA
} HLPackageType;

typedef enum
{
	HL_ATTRIBUTE_INVALID = 0,
	HL_ATTRIBUTE_BOOLEAN,
	HL_ATTRIBUTE_INTEGER,
	HL_ATTRIBUTE_UNSIGNED_INTEGER,
	HL_ATTRIBUTE_FLOAT,
	HL_ATTRIBUTE_STRING
} HLAttributeType;

typedef enum
{
	HL_BSP_PACKAGE_VERSION = 0,
	HL_BSP_PACKAGE_COUNT,
	HL_BSP_ITEM_WIDTH = 0,
	HL_BSP_ITEM_HEIGHT,
	HL_BSP_ITEM_PALETTE_ENTRIES,
	HL_BSP_ITEM_COUNT,

	HL_GCF_PACKAGE_VERSION = 0,
	HL_GCF_PACKAGE_ID,
	HL_GCF_PACKAGE_ALLOCATED_BLOCKS,
	HL_GCF_PACKAGE_USED_BLOCKS,
	HL_GCF_PACKAGE_BLOCK_LENGTH,
	HL_GCF_PACKAGE_LAST_VERSION_PLAYED,
	HL_GCF_PACKAGE_COUNT,
	HL_GCF_ITEM_ENCRYPTED = 0,
	HL_GCF_ITEM_COPY_LOCAL,
	HL_GCF_ITEM_OVERWRITE_LOCAL,
	HL_GCF_ITEM_BACKUP_LOCAL,
	HL_GCF_ITEM_FLAGS,
	HL_GCF_ITEM_FRAGMENTATION,
	HL_GCF_ITEM_COUNT,

	HL_NCF_PACKAGE_VERSION = 0,
	HL_NCF_PACKAGE_ID,
	HL_NCF_PACKAGE_LAST_VERSION_PLAYED,
	HL_NCF_PACKAGE_COUNT,
	HL_NCF_ITEM_ENCRYPTED = 0,
	HL_NCF_ITEM_COPY_LOCAL,
	HL_NCF_ITEM_OVERWRITE_LOCAL,
	HL_NCF_ITEM_BACKUP_LOCAL,
	HL_NCF_ITEM_FLAGS,
	HL_NCF_ITEM_COUNT,

	HL_PAK_PACKAGE_COUNT = 0,
	HL_PAK_ITEM_COUNT = 0,

	HL_SGA_PACKAGE_VERSION_MAJOR = 0,
	HL_SGA_PACKAGE_VERSION_MINOR,
	HL_SGA_PACKAGE_MD5_FILE,
	HL_SGA_PACKAGE_NAME,
	HL_SGA_PACKAGE_MD5_HEADER,
	HL_SGA_PACKAGE_COUNT,
	HL_SGA_ITEM_SECTION_ALIAS = 0,
	HL_SGA_ITEM_SECTION_NAME,
	HL_SGA_ITEM_MODIFIED,
	HL_SGA_ITEM_TYPE,
	HL_SGA_ITEM_CRC,
	HL_SGA_ITEM_VERIFICATION,
	HL_SGA_ITEM_COUNT,

	HL_VBSP_PACKAGE_VERSION = 0,
	HL_VBSP_PACKAGE_MAP_REVISION,
	HL_VBSP_PACKAGE_COUNT,
	HL_VBSP_ITEM_VERSION = 0,
	HL_VBSP_ITEM_FOUR_CC,
	HL_VBSP_ZIP_PACKAGE_DISK,
	HL_VBSP_ZIP_PACKAGE_COMMENT,
	HL_VBSP_ZIP_ITEM_CREATE_VERSION,
	HL_VBSP_ZIP_ITEM_EXTRACT_VERSION,
	HL_VBSP_ZIP_ITEM_FLAGS,
	HL_VBSP_ZIP_ITEM_COMPRESSION_METHOD,
	HL_VBSP_ZIP_ITEM_CRC,
	HL_VBSP_ZIP_ITEM_DISK,
	HL_VBSP_ZIP_ITEM_COMMENT,
	HL_VBSP_ITEM_COUNT,

	HL_VPK_PACKAGE_Archives = 0,
	HL_VPK_PACKAGE_Version,
	HL_VPK_PACKAGE_COUNT,
	HL_VPK_ITEM_PRELOAD_BYTES = 0,
	HL_VPK_ITEM_ARCHIVE,
	HL_VPK_ITEM_CRC,
	HL_VPK_ITEM_COUNT,

	HL_WAD_PACKAGE_VERSION = 0,
	HL_WAD_PACKAGE_COUNT,
	HL_WAD_ITEM_WIDTH = 0,
	HL_WAD_ITEM_HEIGHT,
	HL_WAD_ITEM_PALETTE_ENTRIES,
	HL_WAD_ITEM_MIPMAPS,
	HL_WAD_ITEM_COMPRESSED,
	HL_WAD_ITEM_TYPE,
	HL_WAD_ITEM_COUNT,

	HL_XZP_PACKAGE_VERSION = 0,
	HL_XZP_PACKAGE_PRELOAD_BYTES,
	HL_XZP_PACKAGE_COUNT,
	HL_XZP_ITEM_CREATED = 0,
	HL_XZP_ITEM_PRELOAD_BYTES,
	HL_XZP_ITEM_COUNT,

	HL_ZIP_PACKAGE_DISK = 0,
	HL_ZIP_PACKAGE_COMMENT,
	HL_ZIP_PACKAGE_COUNT,
	HL_ZIP_ITEM_CREATE_VERSION = 0,
	HL_ZIP_ITEM_EXTRACT_VERSION,
	HL_ZIP_ITEM_FLAGS,
	HL_ZIP_ITEM_COMPRESSION_METHOD,
	HL_ZIP_ITEM_CRC,
	HL_ZIP_ITEM_DISK,
	HL_ZIP_ITEM_COMMENT,
	HL_ZIP_ITEM_COUNT
} HLPackageAttribute;

typedef enum
{
	HL_VALIDATES_OK = 0,
	HL_VALIDATES_ASSUMED_OK,
	HL_VALIDATES_INCOMPLETE,
	HL_VALIDATES_CORRUPT,
	HL_VALIDATES_CANCELED,
	HL_VALIDATES_ERROR
} HLValidation;

typedef struct
{
	HLAttributeType eAttributeType;
	hlChar lpName[252];
	union
	{
		struct
		{
			hlBool bValue;
		} Boolean;
		struct
		{
			hlInt iValue;
		} Integer;
		struct
		{
			hlUInt uiValue;
			hlBool bHexadecimal;
		} UnsignedInteger;
		struct
		{
			hlFloat fValue;
		} Float;
		struct
		{
			hlChar lpValue[256];
		} String;
	} Value;
} HLAttribute;

typedef hlVoid HLDirectoryItem;
typedef hlVoid HLStream;

typedef hlVoid (*PExtractItemStartProc) (const HLDirectoryItem *pItem);
typedef hlVoid (*PExtractItemEndProc) (const HLDirectoryItem *pItem, hlBool bSuccess);
typedef hlVoid (*PExtractFileProgressProc) (const HLDirectoryItem *pFile, hlUInt uiBytesExtracted, hlUInt uiBytesTotal, hlBool *pCancel);
typedef hlVoid (*PValidateFileProgressProc) (const HLDirectoryItem *pFile, hlUInt uiBytesValidated, hlUInt uiBytesTotal, hlBool *pCancel);

#ifdef __cplusplus
}
#endif

#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#	define PATH_SEPARATOR_CHAR '\\'
#	define PATH_SEPARATOR_STRING "\\"
#else
#	define PATH_SEPARATOR_CHAR '/'
#	define PATH_SEPARATOR_STRING "/"
#endif

#endif // HLTYPE_H
