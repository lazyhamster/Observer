/****************************************************************************/
/*  This file is part of PPMdMem/C project, 2001                            */
/*  Designed and written by Dmitry Shkarin by request of Alexey Krivonogov  */
/*  Contents: common encoding/decoding routines                             */
/****************************************************************************/
#include "PPMdCmn.h"

//void WINAPI PPMdPrintInfo(M_STREAM* Decoded,M_STREAM* Encoded)
//{
//    if ( ScreenOutputAllowed )
//        printf("Enc. position: %8d, Dec. position: %8d, Memory used: %6dKB\r",
//            Encoded->cp,Decoded->cp,PPMdGetUsedMemory() >> 10);
//}
/*static void __cdecl Error(const char* FmtStr,...)
{
    va_list argptr;
    va_start(argptr,FmtStr);
    vprintf(FmtStr,argptr);
    va_end(argptr);
    exit(-1);
}
*/
void M_INIT_STR( M_STREAM* ms, char* buf, long sz, MS_HANDLER p )
{
  (ms)->cp=0;
  (ms)->user_data=NULL;
  (ms)->pf=NULL;
  (ms)->buf_size=(ms)->n_avail=sz;
  (ms)->buf= buf;//(BYTE*)mem_alloc( sz );
  (ms)->pf = p;
}

int MGetFill(M_STREAM* ms)
{
    if (!ms->pf || (ms->n_avail=ms->pf(ms,PPMD_FULLINPUT)) <= 0)
            return EOS;
    ms->cp=1;                               return ms->buf[0];
}
int MPutFlush(M_STREAM* ms,int c)
{
    if (!ms->pf || (ms->n_avail=ms->pf(ms,PPMD_FULLOUTPUT)) <= 0)
            return EOS;
    if (c == EOS) {
        ms->n_avail=ms->cp=0;               return EOS;
    }
    ms->cp=1;                               return (ms->buf[0]=c);
}

#pragma pack(1)
struct MEM_BLK {
    WORD Stamp, NU;
    struct MEM_BLK* next, * prev;
} _PACK_ATTR;
#pragma pack()
BYTE Indx2Units[N_INDEXES], Units2Indx[128];
NODE FreeList[N_INDEXES];
BYTE* HeapStart, * pText, * UnitsStart, * LoUnit, * HiUnit;
long SubAllocatorSize=0;
static BYTE GlueCount;

/*DWORD WINAPI PPMdGetUsedMemory(void)
{
    DWORD i, k, RetVal=SubAllocatorSize-(HiUnit-LoUnit)-(UnitsStart-pText);
    NODE* pn;
    for (k=i=0;i < N_INDEXES;i++, k=0) {
        for (pn=FreeList+i;(pn=pn->next) != NULL;k++)
                ;
        RetVal -= UNIT_SIZE*Indx2Units[i]*k;
    }
    return RetVal;
}*/

void WINAPI PPMdStopSubAllocator(void)
{
    if ( SubAllocatorSize ) {
        SubAllocatorSize=0; 
        mem_free( HeapStart );
    }
}
BOOL WINAPI PPMdStartSubAllocator(int SASize)
{
    DWORD t=SASize << 20;
    if (SubAllocatorSize == t)              return TRUE;
    PPMdStopSubAllocator();
    if ((HeapStart=(BYTE*) mem_alloc( t )) == NULL )
            return FALSE;
    SubAllocatorSize=t;                     return TRUE;
}
static void InitSubAllocator(void)
{
    UINT Diff=UNIT_SIZE*(SubAllocatorSize/8/UNIT_SIZE*7);
    ZeroMemory(FreeList,sizeof(FreeList));
    HiUnit=(pText=HeapStart)+SubAllocatorSize;
    LoUnit=UnitsStart=HiUnit-Diff;          GlueCount=0;
}
static void GlueFreeBlocks(void)
{
    NODE* tmp;
    struct MEM_BLK s0, * p, * p1;
    int i, k, sz;
    if (LoUnit != HiUnit)                   *LoUnit=0;
    for (i=0, s0.next=s0.prev=&s0;i < N_INDEXES;i++)
            while ( FreeList[i].next ) {
                REMOVE_NODE(tmp,i);         p=(struct MEM_BLK*) tmp;
                p->prev=&s0;                p->next=s0.next;
                s0.next=p->next->prev=p;
                p->Stamp=0xFFFF;            p->NU=Indx2Units[i];
            }
    for (p=s0.next;p != &s0;p=p->next)
        while ((p1=p+p->NU)->Stamp == 0xFFFF && p->NU+(int) p1->NU < 0x10000) {
            p1->prev->next=p1->next;        p1->next->prev=p1->prev;
            p->NU += p1->NU;
        }
    while ((p=s0.next) != &s0) {
        p->prev->next=p->next;              p->next->prev=p->prev;
        for (sz=p->NU;sz > 128;sz -= 128, p += 128) {
            INSERT_NODE(p,N_INDEXES-1);
        }
        if (Indx2Units[i=Units2Indx[sz-1]] != sz) {
            k=sz-Indx2Units[--i];           INSERT_NODE(p+(sz-k),k-1);
        }
        INSERT_NODE(p,i);
    }
}
void* AllocUnitsRare(int indx)
{
    NODE* p;
    int i=indx;
    if ( !GlueCount ) {
        GlueCount = 255;                    GlueFreeBlocks();
        if ( FreeList[indx].next ) {
            REMOVE_NODE(p,indx);            return p;
        }
    }
    do {
        if (++i == N_INDEXES) {
            GlueCount--;                    i=U2B(Indx2Units[indx]);
            return (UnitsStart-pText > i)?(UnitsStart -= i):(NULL);
        }
    } while ( !FreeList[i].next );
    REMOVE_NODE(p,i);                       SPLIT_BLOCK(p,i,indx);
    return p;
}

SEE2_CONTEXT SEE2Cont[24][32], DummySEE2Cont = {0};
PPM_CONTEXT* MaxContext;
BYTE NS2Indx[256], NS2BSIndx[256], Freq2Indx[197], HB2Flag[256];
STATE* FoundState;
int  InitEsc, OrderFall, RunLength, InitRL, MaxOrder, PrintCount;
BYTE CharMask[256], NumMasked, PrevSuccess, EscCount;
WORD BinSumm[25][64];

static void PPMdStartUp(void)               /* constants initialization     */
{
    int i, k, m, Step;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
    NS2BSIndx[0]=2*0;                       NS2BSIndx[1]=2*1;
    memset(NS2BSIndx+2,2*2,9);              memset(NS2BSIndx+11,2*3,256-11);
    NS2Indx[0]=0;                           NS2Indx[1]=1;
    for (m=i=2, k=Step=1;i < 256;i++) {
        NS2Indx[i]=m;
        if ( !--k ) { k = ++Step;           m++; }
    }
    for (i=0;i < UP_FREQ;i++)               Freq2Indx[i]=i;
    for (m=i=UP_FREQ, k=Step=1;i < 196;i++) {
        Freq2Indx[i]=m;
        if ( !--k ) { k = ++Step;           m++; }
    }
    Freq2Indx[i]=m+1;
    memset(HB2Flag,0,0x40);             	memset(HB2Flag+0x40,0x08,0x100-0x40);
    DummySEE2Cont.Shift=PERIOD_BITS;
}
   
static void RestartModelRare(void)
{
    static const WORD InitBinEsc[]={0x3CDD,0x1F3F,0x59BF,0x48F3,0x64A1,0x5ABC,0x6632,0x6051};
    int i, k, m, n;
    if ( !DummySEE2Cont.Shift )             PPMdStartUp();
    memset(CharMask,0,sizeof(CharMask));
    RunLength=InitRL=-((MaxOrder < 12)?MaxOrder:12)-1;
    InitSubAllocator();
    MaxContext = (PPM_CONTEXT*) (HiUnit -= UNIT_SIZE);
    MaxContext->Suffix=NULL;                OrderFall=MaxOrder;
    MaxContext->SummFreq=(MaxContext->NumStats=255)+2;
    FoundState=MaxContext->Stats=(STATE*) LoUnit;
    LoUnit += 256/2*UNIT_SIZE;
    for (PrevSuccess=i=0;i < 256;i++) {
        MaxContext->Stats[i].Symbol=i;      MaxContext->Stats[i].Freq=1;
        MaxContext->Stats[i].Successor=NULL;
    }
    for (i=m=0;m < 25;m++) {
        while (Freq2Indx[i] == m)           i++;
        for (k=0;k < 8;k++)
            for (n=0;n < 64;n += 8)
                    BinSumm[m][k+n]=BIN_SCALE-InitBinEsc[k]/(i+1);
    }
    for (i=m=0;m < 24;m++) {
        while (NS2Indx[i] == m)             i++;
        for (k=0;k < 32;k++) {
            SEE2Cont[m][k].Shift=PERIOD_BITS-4;
            SEE2Cont[m][k].Summ=(2*i+5) << SEE2Cont[m][k].Shift;
            SEE2Cont[m][k].Count=7;
        }
    }
}
void StartModelRare(int iMaxOrder)
{
    PPM_CONTEXT* pc;
    EscCount=1;                             PrintCount=PROGRESS_SIZE;
    if (iMaxOrder < 2) {                    /* solid mode                   */
        memset(CharMask,0,sizeof(CharMask));
        pc=MaxContext;                      OrderFall=MaxOrder;
        while ( pc->Suffix ) {
            pc=pc->Suffix;                  OrderFall--;
        }
        FoundState=pc->Stats;
    } else { MaxOrder=iMaxOrder;            RestartModelRare(); }
}
void RescaleStatistics(PPM_CONTEXT* pc)
{
    int OldNS=pc->NumStats, i=pc->NumStats, n0, n1, Adder, EscFreq;
    STATE* p1, * p;
    for (p=FoundState;p != pc->Stats;p--)   _PPMD_SWAP(p[0],p[-1]);
    pc->Stats->Freq += 4;                   pc->SummFreq += 4;
    EscFreq=pc->SummFreq-p->Freq;           Adder=(OrderFall != 0);
    pc->SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
    do {
        EscFreq -= (++p)->Freq;
        pc->SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) {
            STATE tmp=*(p1=p);
            do {
                p1[0]=p1[-1];
            } while (--p1 != pc->Stats && tmp.Freq > p1[-1].Freq);
            *p1=tmp;
        }
    } while ( --i );
    if (p->Freq == 0) {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;
        if ((pc->NumStats -= i) == 0) {
            STATE tmp=*(pc->Stats);
            tmp.Freq=(2*tmp.Freq+EscFreq-1)/EscFreq;
            if (tmp.Freq > MAX_FREQ/3)      tmp.Freq=MAX_FREQ/3;
            INSERT_NODE(pc->Stats,Units2Indx[OldNS >> 1])
            *(FoundState=ONE_STATE(pc))=tmp;
            pc->Flags=(pc->Flags & 0x10)+HB2Flag[tmp.Symbol];
            return;
        }
        n0=(OldNS+2) >> 1;                  n1=(pc->NumStats+2) >> 1;
        if (n0 != n1 && (n0=Units2Indx[n0-1]) != (n1=Units2Indx[n1-1])) {
            if ( FreeList[n1].next ) {
                REMOVE_NODE(p,n1);
                memcpy(p,pc->Stats,U2B(Indx2Units[n1]));
                INSERT_NODE(pc->Stats,n0);  pc->Stats=p;
            } else { SPLIT_BLOCK(pc->Stats,n0,n1); }
        }
        pc->Flags &= ~0x08;
        for (i=pc->NumStats;i >= 0;i--)
                pc->Flags |= HB2Flag[pc->Stats[i].Symbol];
    }
    pc->SummFreq += (EscFreq -= (EscFreq >> 1));
    pc->Flags |= 0x04;                      FoundState=pc->Stats;
}
static PPM_CONTEXT* CreateSuccessors(BOOL Skip,STATE* p1,PPM_CONTEXT* MinContext)
{
// static UpState declaration bypasses IntelC bug
    static STATE UpState;
    PPM_CONTEXT* pc1, * pc=MinContext, * UpBranch=FoundState->Successor;
    STATE * p, * ps[MAX_O], ** pps=ps;
    UINT cf, s0;
    BYTE Flag;
    if ( !Skip ) {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  goto NO_LOOP;
    }
    if ( p1 ) {
        p=p1;                               pc=pc->Suffix;
        goto LOOP_ENTRY;
    }
    do {
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != FoundState->Symbol)
                do { p++; } while (p->Symbol != FoundState->Symbol);
        } else                              p=ONE_STATE(pc);
LOOP_ENTRY:
        if (p->Successor != UpBranch) {
            pc=p->Successor;                break;
        }
        *pps++ = p;
    } while ( pc->Suffix );
NO_LOOP:
    if (pps == ps)                          return pc;
    UpState.Symbol=*(BYTE*) UpBranch;
    UpState.Successor=(PPM_CONTEXT*) (((BYTE*) UpBranch)+1);
    if ( pc->NumStats ) {
        if ((p=pc->Stats)->Symbol != UpState.Symbol)
                do { p++; } while (p->Symbol != UpState.Symbol);
        cf=p->Freq-1;                       s0=pc->SummFreq-pc->NumStats-cf;
        UpState.Freq=1+((2*cf <= s0)?(5*cf > s0):((cf+2*s0-3)/s0));
    } else                                  UpState.Freq=ONE_STATE(pc)->Freq;
    Flag=HB2Flag[UpState.Symbol]+2*HB2Flag[FoundState->Symbol];
    do {
        if (HiUnit != LoUnit)
                pc1=(PPM_CONTEXT*) (HiUnit -= UNIT_SIZE);
        else if ( FreeList->next ) {
                REMOVE_NODE(pc1,0);
        } else if ((pc1=(PPM_CONTEXT*) AllocUnitsRare(0)) == NULL)
                return NULL;
        pc1->NumStats=0;                    pc1->Flags=Flag;
        *ONE_STATE(pc1)=UpState;            pc1->Suffix=pc;
        (*--pps)->Successor=pc=pc1;
    } while (pps != ps);
    return pc;
}
void UpdateModel(PPM_CONTEXT* MinContext)
{
    STATE fs = *FoundState, * p = NULL;
    PPM_CONTEXT* pc, * Successor;
    UINT i0, i1, i, ns1, ns, cf, sf, s0;
    BYTE Flag;
    if (fs.Freq < MAX_FREQ/4 && (pc=MinContext->Suffix) != NULL) {
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != fs.Symbol) {
                do { p++; } while (p->Symbol != fs.Symbol);
                if (p[0].Freq >= p[-1].Freq) {
                    _PPMD_SWAP(p[0],p[-1]); p--;
                }
            }
            if (p->Freq < MAX_FREQ-9) {
                p->Freq += 2;               pc->SummFreq += 2;
            }
        } else {
            p=ONE_STATE(pc);                p->Freq += (p->Freq < 32);
        }
    }
    if ( !OrderFall ) {
        MaxContext=FoundState->Successor=CreateSuccessors(TRUE,p,MinContext);
        if ( !MaxContext )                  goto RESTART_MODEL;
        return;
    }
    *pText++ = fs.Symbol;                   Successor = (PPM_CONTEXT*) pText;
    if (pText >= UnitsStart)                goto RESTART_MODEL;
    if ( fs.Successor ) {
        if ((BYTE*) fs.Successor <= pText &&
                (fs.Successor=CreateSuccessors(FALSE,p,MinContext)) == NULL)
                        goto RESTART_MODEL;
        if ( !--OrderFall ) {
            Successor=fs.Successor;         pText -= (MaxContext != MinContext);
        }
    } else {
        FoundState->Successor=Successor;    fs.Successor=MinContext;
    }
    Flag=HB2Flag[fs.Symbol];
    s0=MinContext->SummFreq-(ns=MinContext->NumStats)-fs.Freq;
    for (pc=MaxContext;pc != MinContext;pc=pc->Suffix) {
        if ((ns1=pc->NumStats) != 0) {
            i=(ns1+1) >> 1;
            if ((ns1 & 1) != 0 && (i0=Units2Indx[i-1]) != (i1=Units2Indx[i])) {
                if ( FreeList[i1].next ) {  REMOVE_NODE(p,i1);
                } else {
                    p=(STATE*) LoUnit;      LoUnit += U2B(Indx2Units[i1]);
                    if (LoUnit > HiUnit) {
                        LoUnit -= U2B(Indx2Units[i1]);
                        p=AllocUnitsRare(i1);
                        if ( !p )           goto RESTART_MODEL;
                    }
                }
                memcpy(p,pc->Stats,U2B(i)); INSERT_NODE(pc->Stats,i0);
                pc->Stats=p;
            }
            pc->SummFreq += (3*ns1+1 < ns);
        } else {
            if ( FreeList[0].next ) {       REMOVE_NODE(p,0);
            } else {
                p=(STATE*) LoUnit;          LoUnit += UNIT_SIZE;
                if (LoUnit > HiUnit) {
                    LoUnit -= UNIT_SIZE;    p=AllocUnitsRare(0);
                    if ( !p )               goto RESTART_MODEL;
                }
            }
            *p=*ONE_STATE(pc);              pc->Stats=p;
            if (p->Freq < MAX_FREQ/4-1)     p->Freq += p->Freq;
            else                            p->Freq  = MAX_FREQ-4;
            pc->SummFreq=p->Freq+InitEsc+(ns > 2);
        }
        cf=2*fs.Freq*(pc->SummFreq+6);      sf=s0+pc->SummFreq;
        if (cf < 6*sf) {
            cf=1+(cf > sf)+(cf >= 4*sf);
            pc->SummFreq += 4;
        } else {
            cf=4+(cf > 9*sf)+(cf > 12*sf)+(cf > 15*sf);
            pc->SummFreq += cf;
        }
        p=pc->Stats+(++pc->NumStats);       p->Successor=Successor;
        p->Symbol = fs.Symbol;              p->Freq = cf;
        pc->Flags |= Flag;
    }
    MaxContext=fs.Successor;                return;
RESTART_MODEL:
    RestartModelRare();                     PrintCount=EscCount=0;
}

const BYTE ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
//BYTE ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };