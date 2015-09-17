/******************************************************************************
*
* Copyright (C) 2009, The Gentee Group. All rights reserved. 
* This file is part of the Gentee open source project - http://www.gentee.com. 
* 
* THIS FILE IS PROVIDED UNDER THE TERMS OF THE GENTEE LICENSE ("AGREEMENT"). 
* ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE CONSTITUTES RECIPIENTS 
* ACCEPTANCE OF THE AGREEMENT.
*
* Author: Alexey Krivonogov ( gentee )
*
******************************************************************************/

#ifndef _GEA_
#define _GEA_

#include "windows.h"
#include "common/types.h"

   #ifdef __cplusplus
      extern "C" {
   #endif // __cplusplus

#define STACKAPI __fastcall

typedef    unsigned long  dword;
typedef    unsigned short word;
typedef    unsigned char* pbyte;
typedef    dword*         pdword;
typedef    word*          pword;

#include "memory.h"

typedef uint  ( __cdecl *gea_call )( uint, puint, ... );

//--------------------------------------------------------------------------
// Функция обратной связи при упаковке/распаковке
#ifndef UNPPMD
dword STDCALL fgeauser( dword param, dword userfunc, dword pgeaparam );
#endif
//--------------------------------------------------------------------------

   #ifdef __cplusplus
      }
   #endif // __cplusplus

#endif // _GEA_

