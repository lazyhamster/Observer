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

#include "gea.h"

//--------------------------------------------------------------------------

pvoid STDCALL mem_alloc( dword size )
{
   return VirtualAlloc( NULL, size, 
                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
}

//--------------------------------------------------------------------------

void STDCALL mem_free( pvoid ptr )
{
   VirtualFree( ptr, 0, MEM_RELEASE );
}

//--------------------------------------------------------------------------

pvoid  STDCALL mem_zero( pvoid dest, long len )
{
   pdword  ddest = ( pdword )dest;
   long    dwlen = len >> 2;

   while ( dwlen-- ) 
      *ddest++ = 0;
   
   len &= 0x3;
   while ( len-- )
      *((pbyte)ddest)++ = 0;

   return dest;
}

//--------------------------------------------------------------------------

pvoid STDCALL mem_allocz( dword size )
{
   pvoid ptr = mem_alloc( size );
   return mem_zero( ptr, size );
}

//--------------------------------------------------------------------------

pvoid STDCALL mem_copy( pvoid dest, pvoid src, long len )
{
   pdword dsrc = ( pdword )src;
   pdword ddest = ( pdword )dest;
   long   dwlen = len >> 2;

   while ( dwlen-- ) 
      *ddest++ = *dsrc++;
   
   len &= 0x3;
   while ( len-- )
      *((pbyte)ddest)++ = *((pbyte)dsrc)++;

   return dest;
}

//--------------------------------------------------------------------------
