/*** 20.08.99 ******** memory.h ********************************************
*                                                                          *
*    Copyright (c) 1998 NameCompany                                        *
*    Author: Alexey Krivonogov                                             *
*    All rights reserved.                                                  *
*                                                                          *
*    Функции для работы с памятью                                          *
*                                                                          *
***************************************************************************/

#ifndef _MEMORY_
#define _MEMORY_

   #ifdef __cplusplus               
      extern "C" {                 
   #endif // __cplusplus      

//--------------------------------------------------------------------------

pvoid STDCALL mem_alloc( dword size );
pvoid STDCALL mem_allocz( dword size );
pvoid STDCALL mem_copy( pvoid dest, pvoid src, long len );
void  STDCALL mem_free( pvoid ptr );
pvoid STDCALL mem_zero( pvoid dest, long len );

//--------------------------------------------------------------------------

   #ifdef __cplusplus              
      }                            
   #endif // __cplusplus

#endif // _MEMORY_
