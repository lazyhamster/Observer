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

gea_call  callfunc;

BOOL gea_init( uint call )
{
   callfunc = (gea_call)call;

   return TRUE;
}

dword STDCALL fgeauser( dword param, dword userfunc, dword pgeaparam )
{
   dword  result;

   if ( userfunc )
   {
      *( pdword )pgeaparam = param;
      if ( callfunc )
         callfunc( userfunc, &result, 200, pgeaparam );
   }
   return 1;
}

//--------------------------------------------------------------------------

//BOOL WINAPI DllMain( HINSTANCE hinstDll, DWORD fdwReason,
//                       LPVOID lpReserved )
//{
/*   if ( fdwReason == DLL_PROCESS_ATTACH )
   {
//      handledll = hinstDll;
//      fgeauser = &fuser;
   }*/
   //return ( TRUE );
//}

//--------------------------------------------------------------------------

