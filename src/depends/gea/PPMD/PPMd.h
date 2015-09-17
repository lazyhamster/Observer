/****************************************************************************/
/*  This file is part of PPMdMem/C project, 2001                            */
/*  Designed and written by Dmitry Shkarin by request of Alexey Krivonogov  */
/*  Contents: interface to encoding/decoding routines                       */
/****************************************************************************/
#if !defined(_PPMD_H_)
#define _PPMD_H_
#ifndef UNPPMD
#include <windows.h>
#include <stdlib.h>
#include "../gea.h"
#else
//   #include "..\GEA\gea.h"
   #define WINAPI __stdcall
   typedef unsigned long       DWORD;
   typedef int                 BOOL;
   typedef unsigned char       BYTE;
   typedef unsigned short      WORD;

   #define UINT DWORD
   #ifdef __cplusplus
   #define NULL    0
   #else
   #define NULL    ((void *)0)
   #endif
   #define TRUE 1
   #define FALSE 0
   typedef pvoid ( STDCALL* MEMALLOC )( dword size );
   typedef void  ( STDCALL* MEMFREE )( pvoid ptr );       
   typedef dword ( STDCALL* GEAUSER )( dword, dword, dword );
   typedef pvoid ( STDCALL* MEMZERO )( pvoid, dword );
   typedef pvoid ( STDCALL* MEMCOPY )( pvoid, pvoid, dword );


   extern MEMALLOC mem_alloc;
   extern MEMFREE  mem_free;
   extern MEMZERO  ZeroMemory;
   extern MEMCOPY  memcpy;
   extern GEAUSER  fgeauser;

#endif
#ifdef  __cplusplus
extern "C" {
#endif

BOOL  WINAPI PPMdStartSubAllocator(int SubAllocatorSize); /* in MB          */
void  WINAPI PPMdStopSubAllocator(void);    /* it can be called once        */
DWORD WINAPI PPMdGetUsedMemory(void);       /* for information only         */

#define PPMD_FULLINPUT  1
#define PPMD_FULLOUTPUT 2
#define PROGRESS_SIZE   0x7FFF              /* counts number of escapes     */

/* stream handler function definition, it must return number of valid       */
/* (for reading/writing) bytes in the buffer                                */
struct M_STREAM_s;
typedef int (WINAPI* MS_HANDLER) (struct M_STREAM_s* ms,int cmd);

/* memory stream definition: cp, n_avail, buf_size - current free position  */
/* (unread or unwrote) in the buffer, whole number of valid positions in    */
/* the buffer, buffer size; pf - pointer to the handler function for buffer */
/* filling/flushing; user_data - pointer to user variables                  */
typedef struct M_STREAM_s {
    int cp, n_avail, buf_size;
    BYTE* buf;
    MS_HANDLER pf;
    void* user_data;
} M_STREAM;

#define EOS (-1)
//#define M_INIT_MEM(ms,sz)   ((ms)->cp=0, (ms)->user_data=NULL, (ms)->pf=NULL,\
//        (ms)->buf=(BYTE*) malloc((ms)->buf_size=(ms)->n_avail=sz))
//#define M_INIT_STR(ms,sz,p) ((ms)->n_avail=(ms)->cp=0, (ms)->user_data=NULL, \
//        (ms)->pf=p, (ms)->buf=(BYTE*) malloc((ms)->buf_size=sz))

//void M_INIT_STR( M_STREAM* ms, long sz, MS_HANDLER p );
void M_INIT_STR( M_STREAM* ms, char* buf, long sz, MS_HANDLER p );

/*  Example of call sequence:                                               */
/*  Encoding:                               Decoding:                       */
/*  PPMdStartSubAllocator(12);              PPMdStartSubAllocator(12);      */
/*  PPMdEncode(Enc1,Dec1,5);                PPMdDecode(Dec1,Enc1,5);        */
/*  PPMdEncode(Enc2,Dec2,6);                PPMdDecode(Dec2,Enc2,6);        */
/*  PPMdStartSubAllocator(16);              PPMdStartSubAllocator(16);      */
/*  PPMdEncode(Enc2,Dec3,8);                PPMdDecode(Dec3,Enc2,8);        */
/*  PPMdStopSubAllocator();                 PPMdStopSubAllocator();         */
/*  stream Dec1 will be encoded to stream Enc1 with -o5 -m12                */
/*  stream Dec2 will be encoded to stream Enc2 with -o6 -m12                */
/*  stream Dec3 will be encoded and appended to stream Enc2 with -o8 -m16   */
BOOL WINAPI PPMdEncode(M_STREAM* Encoded,M_STREAM* Decoded,int MaxOrder);
BOOL WINAPI PPMdDecode(M_STREAM* Decoded,M_STREAM* Encoded,int MaxOrder);

/* this imported function can be used for drawing progress bar, for example */
void WINAPI PPMdPrintInfo(M_STREAM* Decoded,M_STREAM* Encoded);

#ifdef  __cplusplus
}
#endif

#endif /* !defined(_PPMD_H_) */
