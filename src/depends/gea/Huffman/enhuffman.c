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

#include "huffman.h"
#include "..\LZGE\lzge.h"

//--------------------------------------------------------------------------

dword STACKAPI huf_bits( pshuf huf, dword item )
{
   register pshufleaf cur = huf->tree + item;
   register dword     depth = 0;

   if ( !cur->up ) return 0;

   do {
      depth++;
      cur = cur->up;
   } while ( cur != huf->root );

   return depth;
}

//--------------------------------------------------------------------------

void  STDCALL huf_entree( pshuf huf )
{
   dword   bits;
   dword   i, k;
   dword   freq[ 2048 ];
   pbyte   st = huf->ptr;
   
   // Вычитаем предыдущие длины

   i = max( 2, lzge_bits( huf->min ));
   bits = lzge_bits( huf->max - huf->min + 1 );
   bits = max( i, bits );

   huf_packnum( huf, bits - 1, huf->numcode < 32 ? 2 : 3 );
//   if ( huf->fixed )
//      printf("Bits=%i min=%i max=%i\n", bits, huf->min, huf->max );
   huf_packnum( huf, huf->min, bits );
   huf_packnum( huf, huf->max - huf->min, bits );
   for ( i = 0; i < huf->numcode; i++ )
   {
      freq[ i ] = huf->tree[ i ].freq;
      // Сдвигаем на минимум
      k = huf->prevlen[ i ];
      huf->prevlen[i] = 0;
      if ( freq[i] )
         freq[ i ] -= huf->min - 1;

         if ( freq[i] <= PRET_MAXCODE )
         {
            huf->prevlen[i] = (byte)freq[i];
            freq[i] = ( freq[i]  - k ) % ( PRET_MAXCODE + 1 );
//            huf->prevlen[i] = freq[i];
         }
//      if ( !huf->fixed && freq[i] > PRET_MAXCODE )
//         printf("%i ", freq[i] );
      
   }
   if ( huf->fixed )
   {
      for ( i = 0; i < huf->numcode; i++ )
         huf_packnum( huf, freq[ i ], bits );
   }      
   else
   {
      shuf   huftree;
      dword  fi = 0;
      
      // Переводим длины в специальное представление
      for ( i = 0; i < huf->numcode; i++ )
      {
         if ( !freq[i] )
         {
            k = i + 1;  
            while ( k < huf->numcode && !freq[k] &&
                  ( k - i < PRET_MAXZERO - 1 ))
               k++;
//          if ( k - i == PRET_MAXZERO - 1 )
//             printf("Len=%i\n", k- i );
            k -= i;
            if ( k > 3 )
            {
               i += k - 1;
               if ( k < PRET_OFF2 )
                  freq[ fi++ ] = (( k - PRET_OFF1 ) << 24 ) | PRET_ZERO2;
               else
                  if ( k < PRET_OFF3 )
                     freq[ fi++ ] = (( k - PRET_OFF2 ) << 24 ) | PRET_ZERO4;
                  else
//                     freq[ fi++ ] = (( k - 16 ) << 24 ) | PRET_ZERO5;
                     freq[ fi++ ] = (( k - PRET_OFF3 ) << 24 ) | PRET_ZERO6;
               continue;
            }
         }
         if ( freq[i] > PRET_MAXCODE )
         {
            freq[ fi++ ] = ( freq[i] << 24 ) | PRET_BIG;
         }
         else
            freq[ fi++ ] = freq[i];
      }
      huf_new( &huftree, PRET_ALPHABET );
      huftree.ptr = huf->ptr;
      huftree.bit = huf->bit;
      huftree.fixed = 1;
      for ( i = 0; i < fi; i++ )
         huf_incfreq( &huftree, freq[ i ] & 0xFF  );

      huf_build( &huftree );
      for ( i = 0; i < fi; i++ )
      {
         huf_outbits( &huftree, freq[ i ] & 0xFF );
         //printf("%i:%i ", i, freq[i] & 0xFF );

         switch ( freq[i] & 0xFF )
         {
            case PRET_ZERO2:
               huf_packnum( &huftree, freq[i] >> 24, 2 );
               break;
            case PRET_ZERO4:
               huf_packnum( &huftree, freq[i] >> 24, 4 );
               break;
//            case PRET_ZERO5:
//               huf_packnum( &huftree, freq[i] >> 24, 5 );
//               break;
            case PRET_ZERO6:
               huf_packnum( &huftree, freq[i] >> 24, 6 );
               break;
            case PRET_BIG:
               huf_packnum( &huftree, freq[i] >> 24, bits );
               break;
         }
      }
      huf->ptr = huftree.ptr;
      huf->bit = huftree.bit;

      huf_destroy( &huftree );
   }
      /* Делаем предыдущую глубину из текущей, если текущая и 
      // предыдущая > PRET_MAXCODE
      if ( i && freq[i] <= PRET_MAXCODE && freq[ i - 1 ] <= PRET_MAXCODE )
      {
//         freq[i] = ( freq[i] + PRET_MAXCODE - freq[ i - 1 ] ) & PRET_MAXCODE;
      }*/
//   if ( !huf->fixed ) printf("\nLen=%i\n", huf->ptr - st );
}

//--------------------------------------------------------------------------

void  STDCALL huf_build( pshuf huf )
{
   pshufleaf  cur = huf->tree;
   pshufleaf  left, end;
   pshufleaf  right;
   dword      i, count = huf->numcode;

   end = huf->tree + huf->allcode - 1;
   while ( cur < end )
   {
      cur->up = NULL;
      cur++;
   }

   while ( 1 )
   {
      // ищем две минимальных частоты
      cur = ( pshufleaf )huf->tree;
      left = NULL;
      right = NULL;

      i = 0;
      while ( i++ < count )
      {
         if ( !cur->up && cur->freq )
//         if ( cur->freq )
         {
            if ( !left || cur->freq < left->freq )
            {
               if ( left && !right )
                  right = left;
               left = cur;
            }
            else
               if ( !right || cur->freq < right->freq )
                  right = cur;
         }
         cur++;
      }
      if ( !right ) break;
      // на основе этих частот составляем третью
      cur = huf->tree + count++;
      cur->freq = left->freq + right->freq;
      cur->left = left;
      cur->right = right;
      left->up = cur;
//      left->freq = 0;

      right->up = cur;
//      right->freq = 0;

      cur->up = NULL;
   }
   if ( count == huf->numcode )
   {  // У нас только один элемент из алфавита
      cur = huf->tree + count++;
      cur->freq = left->freq;
      cur->left = left;
      cur->right = NULL;
      left->up = cur;
      cur->up = NULL;
   }
   huf->root = huf->tree + count - 1;
   huf->min = 0xFF;
   huf->max = 1;

   for (i  = 0; i < huf->allcode; i++ )
   {
      cur = huf->tree + i;
      cur->freq = i < huf->numcode ? huf_bits( huf, i ) : 0 ;
      if ( cur->freq && cur->freq < huf->min ) 
         huf->min = cur->freq;
      if ( cur->freq > huf->max ) 
         huf->max = cur->freq;
   }
   huf_entree( huf );
   huf_normilize( huf );
}

//--------------------------------------------------------------------------

void  STACKAPI huf_incfreq( pshuf huf, dword item )
{
   ( huf->tree + item )->freq++;
}

//--------------------------------------------------------------------------

void STACKAPI huf_outbits( pshuf huf, dword item )
{
   register pshufleaf cur = huf->tree + item;
   register pbyte     ps = huf->stack;

   if ( !cur->up ) return;

   do {
      *ps++ = ( cur->up->right == cur );
      cur = cur->up;
   } while ( cur != huf->root );

   do {
      *huf->ptr = ( *huf->ptr << 1 ) | *--ps;
      if ( !( huf->bit = ( huf->bit + 1 ) & 7 ))
         huf->ptr++;
   } while ( ps > huf->stack );
}

//--------------------------------------------------------------------------

void  STACKAPI huf_packnum( pshuf huf, dword val, dword bits )
{
//   if ( val >= ( dword )(1 << bits )) printf("Ooops %i %i\n", val, bits );
   while ( bits-- )
   {
      *huf->ptr = ( *huf->ptr << 1 ) | ( byte )( val & 1 );

      if ( !( huf->bit = ( huf->bit + 1 ) & 7 ))
         huf->ptr++;

      val >>= 1;
   }
}

//--------------------------------------------------------------------------

