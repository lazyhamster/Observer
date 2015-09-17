/******************************************************************************
*
* Copyright (C) 2006, The Gentee Group. All rights reserved. 
* This file is part of the Gentee open source project - http://www.gentee.com. 
* 
* THIS FILE IS PROVIDED UNDER THE TERMS OF THE GENTEE LICENSE ("AGREEMENT"). 
* ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE CONSTITUTES RECIPIENTS 
* ACCEPTANCE OF THE AGREEMENT.
*
* ID: types 18.10.06 0.0.A.
*
* Author: Alexey Krivonogov
* Contributors: santy
*
* Summary: This file provides basic types and some constants.
*
******************************************************************************/

#ifndef _TYPES_
#define _TYPES_

#if defined (__GNUC__) || defined (__TINYC__)
   #include <_mingw.h>
   #include <stdlib.h>
#endif

   #ifdef __cplusplus               
      extern "C" {                 
   #endif // __cplusplus      

#define STDCALL   __stdcall 
#define FASTCALL  __fastcall
#define CDECLCALL __cdecl

//--------------------------------------------------------------------------
#if defined (__WATCOMC__) || defined (__GNUC__)
 #ifdef BUILD_DLL
  #define DLL_EXPORT __declspec(dllexport)
 #else
  #define DLL_EXPORT
 #endif
#else
 #define DLL_EXPORT
#endif

//--------------------------------------------------------------------------
#if defined ( __GNUC__) || defined (__TINYC__)
   //typedef             char  byte;  // Santy
   //typedef  byte*     pbyte;        // Santy
#endif

typedef    unsigned char  ubyte;    
typedef    unsigned long  uint;
typedef   unsigned short  ushort;
typedef unsigned __int64  ulong64;
typedef          __int64  long64;

typedef  ubyte*    pubyte;
typedef  char *    pchar;
typedef  uint*     puint;
typedef   int*     pint;
typedef  void*     pvoid;
typedef  ushort*   pushort;
typedef  short*    pshort;
typedef  ulong64*  pulong64;
typedef  long64*   plong64;

#define  ABC_COUNT    256
#define  FALSE        0
#define  TRUE         1
#define  MAX_BYTE     0xFF        
#define  MAX_USHORT   0xFFFF      
#define  MAX_UINT     0xFFFFFFFF  
#define  MAX_MSR      8                   // Max array dimension


// File operation flags os_fileopen
#define FOP_READONLY   0x0001   // open as readonly
#define FOP_EXCLUSIVE  0x0002   // open exclusively
#define FOP_CREATE     0x0004   // create file
#define FOP_IFCREATE   0x0008   // create file if it doesn't exist

// File position os_fileset
#define FSET_BEGIN     0x0001   
#define FSET_CURRENT   0x0002   
#define FSET_END       0x0003   

#define LOBY( x )    ( x & 0xFF )
#define HIBY( x )    (( x >> 8 ) & 0xFF )

typedef  int  ( __stdcall* cmpfunc)( const pvoid, const pvoid, uint );

//--------------------------------------------------------------------------

   #ifdef __cplusplus              
      }                            
   #endif // __cplusplus

#endif // _TYPES_