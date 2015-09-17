/****************************************************************************/
/*  This file is part of PPMdMem/C project, 2001                            */
/*  Designed and written by Dmitry Shkarin by request of Alexey Krivonogov  */
/*  Contents: internal header file, private defs, data types etc.           */
/****************************************************************************/
#if !defined(_PPMDCMN_H_)
#define _PPMDCMN_H_
#include "PPMd.h"

#ifdef  __cplusplus
extern "C" {
#endif
#ifndef UNPPMD
//#include "K:\Gentee\Gentee Language\VM Lib\memory.h"
#else
//   #include "..\GEA\gea.h"
//extern FARPROC mem_alloc;
//extern FARPROC mem_free;
//extern FARPROC fgeauser;
   void   STDCALL memset( pbyte ptr, int val, unsigned int len );
#endif
/* memory stream fill/flush auxilary functions                              */
int MGetFill(M_STREAM* ms);
int MPutFlush(M_STREAM* ms,int c);
#define MGETC(ms)   ((ms->cp < ms->n_avail)?(ms->buf[ms->cp++]):MGetFill(ms))
#define MPUTC(c,ms) ((ms->cp < ms->n_avail)?(ms->buf[ms->cp++]=c):MPutFlush(ms,c))

enum SUB_ALLOCATOR_CONSTS {                 /* memory manager constants     */
    UNIT_SIZE=12, N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4,
    N_INDEXES=N1+N2+N3+N4
};
enum PPMD_CONSTS {
    MAX_O=16,                               /* maximum allowed model order  */
    UP_FREQ=5, MAX_FREQ=124, INT_BITS=7, PERIOD_BITS=7,
    TOT_BITS=INT_BITS+PERIOD_BITS, INTERVAL=1 << INT_BITS,
    BIN_SCALE=1 << TOT_BITS };
enum RANGE_CODER_CONSTS { TOP=1 << 24, BOT=1 << 15 };

/* "#pragma pack" is used for other compilers                               */
#if defined(__GNUC__)
#define _PACK_ATTR __attribute__ ((packed))
#else
#define _PACK_ATTR
#endif /* defined(__GNUC__) */

/* memory manager variables                                                */
extern BYTE Indx2Units[N_INDEXES], Units2Indx[128]; /* constants */
typedef struct N { struct N* next; } NODE;
extern NODE FreeList[N_INDEXES];
extern BYTE* HeapStart, * pText, * UnitsStart, * LoUnit, * HiUnit;
extern long SubAllocatorSize;

/* memory allocation macros                                                */
void* AllocUnitsRare(int indx);
//#define U2B(NU) (8*(NU)+4*(NU))
#define U2B(NU) (((NU)<<3 ) +((NU)<<2))
#define INSERT_NODE(p,i) {                                                  \
    ((NODE*) (p))->next=FreeList[i].next;                                   \
    FreeList[i].next=(NODE*) (p);                                           \
}
#define REMOVE_NODE(p,i) {                                                  \
    ((NODE*) (p))=FreeList[i].next;                                         \
    FreeList[i].next=((NODE*) (p))->next;                                   \
}
#define SPLIT_BLOCK(pv,OldIndx,NewIndx) {                                   \
    int _k, _UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];                 \
    BYTE* _p=((BYTE*) (pv))+U2B(Indx2Units[NewIndx]);                       \
    if (Indx2Units[_k=Units2Indx[_UDiff-1]] != _UDiff) {                    \
        _k--;                                INSERT_NODE(_p,_k);            \
        _k=Indx2Units[_k];                    _p += U2B(_k);                \
        _UDiff -= _k;                                                       \
    }                                                                       \
    INSERT_NODE(_p,Units2Indx[_UDiff-1]);                                   \
}

/* common encoding/decoding data types                                      */
#pragma pack(1)
typedef struct {
    WORD Summ;
    BYTE Shift, Count;
} _PACK_ATTR SEE2_CONTEXT;
struct PPMC;
typedef struct {
    BYTE Symbol, Freq;
    struct PPMC* Successor;
} _PACK_ATTR STATE;
typedef struct PPMC {
    BYTE NumStats, Flags;
    WORD SummFreq;
    STATE* Stats;
    struct PPMC* Suffix;
} _PACK_ATTR PPM_CONTEXT;
#pragma pack()

#define _PPMD_SWAP(t1,t2) { STATE _t=t1; t1=t2; t2=_t; }
#define ONE_STATE(pc) ((STATE*) &(pc->SummFreq))

/* common encoding/decoding variables                                       */
extern BYTE NS2Indx[256], NS2BSIndx[256], Freq2Indx[197], HB2Flag[256]; /* constants */
extern SEE2_CONTEXT SEE2Cont[24][32], DummySEE2Cont;
extern PPM_CONTEXT* MaxContext;
extern STATE* FoundState;
extern int  InitEsc, OrderFall, RunLength, InitRL, MaxOrder, PrintCount;
extern BYTE CharMask[256], NumMasked, PrevSuccess, EscCount;
extern WORD BinSumm[25][64];
extern const BYTE ExpEscape[16];

/* common encoding/decoding routines and macros                             */
void StartModelRare(int iMaxOrder);
void RescaleStatistics(PPM_CONTEXT* pc);
void UpdateModel(PPM_CONTEXT* MinContext);

#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))
#define IS_CORRECT_INPUT(Encoded,Decoded,MaxOrder) (Encoded &&              \
    Encoded->buf && Decoded && Decoded->buf && MaxOrder > 0 &&              \
    MaxOrder <= MAX_O && SubAllocatorSize != 0)
#define CLEAR_MASK(Encoded,Decoded) {                                       \
    memset(CharMask,0,sizeof(CharMask));    EscCount=1;                     \
    if ((PrintCount -= 255) <= 0) {                                       	 \
        PrintCount += PROGRESS_SIZE;       	PPMdPrintInfo(Decoded,Encoded); \
    }                                                                       \
}

#ifdef  __cplusplus
}
#endif

#endif /* !defined(_PPMDCMN_H_) */
