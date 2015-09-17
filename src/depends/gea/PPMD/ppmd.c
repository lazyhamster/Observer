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
//#include "..\gea\gea.h"
#include "ppmdcmn.h"
#include "endeppmd.h"

psppmd  gppmd;

//--------------------------------------------------------------------------

void WINAPI PPMdPrintInfo(M_STREAM* Decoded,M_STREAM* Encoded)
{
   fgeauser( Encoded->cp, gppmd->userfunc, gppmd->pgeaparam );
//        printf("Enc. position: %8d, Dec. position: %8d, Memory used: %6dKB\r",
//            Encoded->cp,Decoded->cp,/*PPMdGetUsedMemory() >> 10*/10);
}

//--------------------------------------------------------------------------

void STDCALL ppmd_stop( void )
{
	PPMdStopSubAllocator( );
}

//--------------------------------------------------------------------------

dword STDCALL ppmd_start( dword memory )
{
	return PPMdStartSubAllocator( memory );
}

//--------------------------------------------------------------------------

