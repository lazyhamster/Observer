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

//#include "..\GEA\gea.h"
//#include "K:\Gentee\Gentee Language\VM Lib\memory.h"
#include "ppmd.h"
#include "endeppmd.h"

//--------------------------------------------------------------------------

dword  STDCALL ppmd_encode( pbyte in, dword size, pbyte out, dword outsize,
                           psppmd ppmd )
{
   M_STREAM  sin, sout;
   
/*   if ( ppmd->memory )
   {
      PPMdStopSubAllocator();
      printf("Alloc %i\n", ppmd->memory );
      PPMdStartSubAllocator( ppmd->memory );
   }*/
   M_INIT_STR( &sin, in, size, 0 );
   M_INIT_STR( &sout, out, outsize, 0 );
   gppmd = ppmd;
//   print( 0, "\ncp=%i size=%i order=%i", sout.cp, size, order );
   size = PPMdEncode( &sout, &sin, ppmd->order );
   return sout.cp;
}

//--------------------------------------------------------------------------

