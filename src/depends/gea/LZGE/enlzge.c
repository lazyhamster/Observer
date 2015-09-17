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

#include "lzge.h"
//#include "..\GEA\gea.h"
//#include "K:\Gentee\Gentee Language\VM Lib\memory.h"

//--------------------------------------------------------------------------

dword  STACKAPI  lzge_slot( pslzge lzge, dword len, dword offset, pdword footer, 
                            pdword lenfooter )
{
   dword i = 0, k = 0;

   while ( rngmax[ i ] < offset ) i++;
   if ( offset == lzge->mft[0] ) i = 0;
   else 
      if ( offset == lzge->mft[1] ) 
      {
         lzge->mft[1] = lzge->mft[ 0 ];
         lzge->mft[0] = offset;
         i = 1;
      }
      else
      {
         if ( offset == lzge->mft[2] ) 
            i = 2;
         lzge->mft[2] = lzge->mft[1];
         lzge->mft[1] = lzge->mft[0];
         lzge->mft[0] = offset;
      }

   *footer = i;
   k = 0;
   while ( lenmax[ k ] < len ) k++;

   *lenfooter = k;
   return LZGE_ALPHABET + i * SLOT_LEN + k;
}

//--------------------------------------------------------------------------

dword  STDCALL lzge_encode( pbyte in, dword size, pbyte out, pslzge lzge )
{
   smatch match;
   shuf   huf;
   dword  numblock = 0;
   dword  len, offset;
   pbyte  end = in + size;
   pdword proceed;
   dword  curp = 0, ip;
   dword  footer, lenfooter;

//   dword ok = 0;

   if ( !lzge->level ) lzge->level = 5;
   if ( !lzge->hufblock ) lzge->hufblock = HUF_BLOCK;
   proceed = mem_alloc(( lzge->hufblock + 1 ) * 3 * sizeof( dword ));
   
   lzge_ranges( lzge, match_new( &match, in, end, min( 10, lzge->level )));
   lzge->mft[0] = 3;
   lzge->mft[1] = 4;
   lzge->mft[2] = 5;
//   lzge->mft[3] = 7;
   if ( lzge->solidoff ) in++;
   for ( ip = 1; ip < lzge->solidoff; ip++ )
   {
      match_update( &match, in++ );
   }

   huf_new( &huf, LZGE_ALPHABET + SLOT_LEN * lzge->numoff + 1 );
//   huf_new( &huf, LZGE_ALPHABET + SLOT_LEN *  + 1 );
//   printf("HUF=%i numoff = %i\n", huf.numcode, lzge->numoff );
//   huf.fixed = 1;
   huf.ptr = out;
   
   while ( in < end )
   {
      len = match_find( &match, in, &offset );
//      ok++;
      if ( len < MATCH_LEN )
      {
         huf_incfreq( &huf, *in );
         proceed[ curp++ ] = *in;
//         printf("%c", *in++ );
      }
      else
      {
         if ( len > lenmax[ SLOT_LEN - 1 ])
         {
            proceed[ curp ] = LZGE_ALPHABET + lzge->numoff * SLOT_LEN;
            lenfooter = SLOT_LEN;
         }
         else
         {
            proceed[ curp ] = lzge_slot( lzge, len, offset, &footer, 
                            &lenfooter );
         }
         huf_incfreq( &huf, proceed[ curp++ ] );
//         if ( lenbits[ lenfooter ])
//         {
            proceed[ curp ] = len - lenmin[ lenfooter ];
            proceed[ curp++ ] |= lenbits[ lenfooter ] << 24;
//         }
         if ( len > lenmax[ SLOT_LEN - 1 ])
         {
            proceed[ curp ] = offset;
            proceed[ curp++ ] |= lzge->maxbit << 24;
         }
         else
         {
            //footer = lzge->mft[ 0 ];
            proceed[ curp ] = offset - rngmin[ footer ];
            proceed[ curp++ ] |= rngbits[ footer ] << 24;
         }
//      if ( size - (end - in) < 700 )
//        printf("%i cur=%i len=%i\n", numblock, size - (end - in), len );
//         if ( curp < 160 )
//           printf("Cur=%i Offset = %i len= %i\n", size - (end - in),  offset, len );
/*                printf("<%i: %i :", offset, len );
         while ( len-- )
         {
            printf("%c", *in++ );
         }
         printf(">");*/
      }
      numblock++;
      in += len;

//      if ( !( size - ( end - in ) & 0xFFF )) 
//         fgeauser( GUProcess, size - ( end - in ), 0 );

      if ( numblock == lzge->hufblock || in == end )
      {
//         printf("Rebuild=%i %i-------------\n", numblock, lzge->hufblock);
         huf_build( &huf );

         ip = 0;
         while ( ip < curp )
         {
            huf_outbits( &huf, proceed[ ip ] );
            if ( proceed[ ip ] >= LZGE_ALPHABET )
            {
               // Упаковываем длину
               if ( proceed[ ++ip ] >> 24 )
                  huf_packnum( &huf, proceed[ ip ] & 0xFFFFFF, proceed[ ip ] >> 24 );
//               if ( ip < 120 )
//               {
//                  printf("off = %i len=%i\n", proceed[ ip + 1 ] & 0xFFFFFF, proceed[ ip ] & 0xFFFFFF );
//               }
               // Упаковываем смещение
               if ( proceed[ ++ip ] >> 24 )
                  huf_packnum( &huf, proceed[ ip ] & 0xFFFFFF, proceed[ ip ] >> 24 );
            }
            ip++;
         } 
         fgeauser( size - ( end - in ) - lzge->solidoff, 
                   lzge->userfunc, lzge->pgeaparam );
         curp = 0;
         numblock = 0;
      }
   }
//   printf("OK=%i %i\n", ok, bits >> 3 );
   // Записываем последний символ
   if ( huf.bit )
      *huf.ptr++ <<= 8 - huf.bit;

   size = huf.ptr - out;
   match_destroy( &match );
   huf_destroy( &huf );
//   fgeauser( GUProcess, size, 0 );

   return size;
}

//--------------------------------------------------------------------------

