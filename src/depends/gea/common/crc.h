/******************************************************************************
*
* Copyright (C) 2006, The Gentee Group. All rights reserved. 
* This file is part of the Gentee open source project - http://www.gentee.com. 
* 
* THIS FILE IS PROVIDED UNDER THE TERMS OF THE GENTEE LICENSE ("AGREEMENT"). 
* ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE CONSTITUTES RECIPIENTS 
* ACCEPTANCE OF THE AGREEMENT.
*
* ID: crc 18.10.06 0.0.A.
*
* Author: Alexey Krivonogov
*
* Summary: This file provides calculating CRC32.
*
******************************************************************************/

#ifndef _CRC_
#define _CRC_

   #ifdef __cplusplus               
      extern "C" {                 
   #endif // __cplusplus      

//--------------------------------------------------------------------------

#include "types.h"

void  STDCALL crc_init( void );
uint  STDCALL crc( pubyte data, uint size, uint seed );

//--------------------------------------------------------------------------

   #ifdef __cplusplus              
      }                            
   #endif // __cplusplus

#endif // _CRC_
