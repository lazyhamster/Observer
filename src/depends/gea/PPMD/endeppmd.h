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

#ifndef _PPMD_
#define _PPMD_

   #include "../gea.h"
   #include "ppmd.h"

   #ifdef __cplusplus
      extern "C" {
   #endif // __cplusplus

//--------------------------------------------------------------------------

typedef struct
{
//   dword  memory;      // Память 
   dword       order;       // Скорость сжатия от 1 до 10
   dword       userfunc;    // Пользовательская функция
   dword       pgeaparam;  // Указатель на geaparam 
} sppmd, * psppmd;

extern psppmd gppmd;

//--------------------------------------------------------------------------

   #ifdef __cplusplus
      }
   #endif // __cplusplus

#endif // _PPMD_

