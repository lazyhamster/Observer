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

#include "crc.h"

uint  _crctbl[ 256 ];

//--------------------------------------------------------------------------

void STDCALL crc_init( void )
{
   uint  reg, polinom, i, ib;

   polinom = 0xEDB88320;
   _crctbl[ 0 ] = 0;

   for ( i = 1; i < 256; i++ )
   {
      reg = 0;
      for ( ib = i | 256; ib != 1; ib >>= 1 )
      {
         reg = ( reg & 1 ? (reg >> 1) ^ polinom : reg >> 1 );
         if ( ib & 1 )
            reg ^= polinom;
      }
      _crctbl[ i ] = reg;
   }
}

//--------------------------------------------------------------------------
// seed must be 0xFFFFFFFF in the first calling

uint STDCALL crc( pubyte data, uint size, uint seed )
{
   while ( size-- )
      seed = _crctbl[((ushort)seed ^ ( *data++ )) & 0xFF ] ^ ( seed >> 8 );

   return seed;
}

//--------------------------------------------------------------------------

