/****************************************************************************/
/*  This file is part of PPMdMem/C project, 2001                            */
/*  Designed and written by Dmitry Shkarin by request of Alexey Krivonogov  */
/*  Contents: decoding routines                                             */
/****************************************************************************/
#include "PPMdCmn.h"

#define ARI_DEC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        code=(code << 8) | MGETC(stream);                                   \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
#define ARI_GET_CURRENT_COUNT() ((code-low)/(range /= scale))
#define ARI_GET_CURRENT_SHIFT_COUNT(SHIFT) ((code-low)/(range >>= SHIFT))
#define ARI_REMOVE_SUBRANGE() {                                             \
    low += range*LowCount;                  range *= HighCount-LowCount;    \
}

BOOL WINAPI PPMdDecode(M_STREAM* Decoded,M_STREAM* Encoded,int MaxOrder)
{
    PPM_CONTEXT* pc;
    STATE* ps[256], ** pps, * p;
    SEE2_CONTEXT* psee;
    WORD* pbs;
    BYTE ns;
    int i, count, Cnt;
    DWORD low=0, code=0, range=-1, LowCount, HighCount, scale;
    if ( !IS_CORRECT_INPUT(Encoded,Decoded,MaxOrder) )
                    return FALSE;
    for (i=0;i < 4;i++)                     code=(code << 8) | MGETC(Encoded);
    StartModelRare(MaxOrder);
    for (ns=(pc=MaxContext)->NumStats; ; ) {
        if ( ns ) {
            p=pc->Stats;                    scale=pc->SummFreq;
            if ((count=ARI_GET_CURRENT_COUNT()) < (Cnt=p->Freq)) {
                PrevSuccess=(((HighCount=p->Freq)<<1) > scale);
                RunLength += PrevSuccess;   LowCount=0;
                (FoundState=p)->Freq += 4;  pc->SummFreq += 4;
                if (p->Freq > MAX_FREQ)     RescaleStatistics(pc);
            } else {
                PrevSuccess=0;              i=pc->NumStats;
                while ((Cnt += (++p)->Freq) <= count)
                    if (--i == 0) {
                        LowCount=Cnt;       CharMask[p->Symbol]=EscCount;
                        i=NumMasked=pc->NumStats;
                        do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
                        FoundState=NULL;    HighCount=scale;
                        goto SYMBOL_NOT_FOUND;
                    }
                LowCount=(HighCount=Cnt)-p->Freq;
                (FoundState=p)->Freq += 4;  pc->SummFreq += 4;
                if (p[0].Freq > p[-1].Freq) {
                    _PPMD_SWAP(p[0],p[-1]); FoundState=--p;
                    if (p->Freq > MAX_FREQ) RescaleStatistics(pc);
                }
            }
SYMBOL_NOT_FOUND:
            ;
        } else {
            p=ONE_STATE(pc);
            pbs=BinSumm[Freq2Indx[p->Freq-1]]+PrevSuccess+
                NS2BSIndx[pc->Suffix->NumStats]+pc->Flags+
                ((RunLength >> 26) & 0x20);
            Cnt=*pbs;
            if (ARI_GET_CURRENT_SHIFT_COUNT(TOT_BITS) < Cnt) {
                FoundState=p;               p->Freq += (p->Freq < 196);
                LowCount=0;                 HighCount=Cnt;
                *pbs = Cnt+INTERVAL-GET_MEAN(Cnt,PERIOD_BITS,2);
                PrevSuccess=1;              RunLength++;
            } else {
                LowCount=Cnt;               *pbs=Cnt-GET_MEAN(Cnt,PERIOD_BITS,2);
                HighCount=BIN_SCALE;        InitEsc=ExpEscape[*pbs >> 10];
                CharMask[p->Symbol]=EscCount;
                NumMasked=PrevSuccess=0;    FoundState=NULL;
            }
        }
        ARI_REMOVE_SUBRANGE();
        while ( !FoundState ) {
            ARI_DEC_NORMALIZE(Encoded);
            do {
                OrderFall++;                pc=pc->Suffix;
                if ( !pc )                  goto STOP_DECODING;
            } while (pc->NumStats == NumMasked);
            if (pc->NumStats != 0xFF) {
                psee=SEE2Cont[NS2Indx[pc->NumStats-1]]+
                    (pc->SummFreq > 11*(pc->NumStats+1))+
                    2*(2*pc->NumStats < pc->Suffix->NumStats+NumMasked)+
                    pc->Flags;
                scale=(psee->Summ >> psee->Shift);
                psee->Summ -= scale;        scale += (scale == 0);
            } else {
                psee=&DummySEE2Cont;        scale=1;
            }
            Cnt=0;                          i=pc->NumStats-NumMasked;
            p=pc->Stats-1;                  pps=ps;
            do {
                do { p++; } while (CharMask[p->Symbol] == EscCount);
                Cnt += p->Freq;             *pps++ = p;
            } while ( --i );
            scale += Cnt;                   count=ARI_GET_CURRENT_COUNT();
            p=*(pps=ps);
            if (count < Cnt) {
                Cnt=0;
                while ((Cnt += p->Freq) <= count)
                        p=*++pps;
                LowCount = (HighCount=Cnt)-p->Freq;
                (FoundState=p)->Freq += 4;
                pc->SummFreq += 4;
                if (p->Freq > MAX_FREQ)     RescaleStatistics(pc);
                EscCount++;                 RunLength=InitRL;
                if (psee->Shift < PERIOD_BITS && --psee->Count == 0) {
                    psee->Summ += psee->Summ;
                    psee->Count=3 << psee->Shift++;
                }
            } else {
                LowCount=Cnt;               HighCount=scale;
                i=pc->NumStats-NumMasked;   pps--;
                do { CharMask[(*++pps)->Symbol]=EscCount; } while ( --i );
                psee->Summ += scale;        NumMasked = pc->NumStats;
            }
            ARI_REMOVE_SUBRANGE();
        }
        MPUTC(FoundState->Symbol,Decoded);
        if (!OrderFall && (BYTE*) FoundState->Successor > pText)
                MaxContext=FoundState->Successor;
        else {
            UpdateModel(pc);
            if (EscCount == 0)              CLEAR_MASK(Encoded,Decoded);
        }
        ns=(pc=MaxContext)->NumStats;
        ARI_DEC_NORMALIZE(Encoded);
    }
STOP_DECODING:
    PPMdPrintInfo(Decoded,Encoded);         MPutFlush(Decoded,EOS);
    return TRUE;
}
