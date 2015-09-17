/****************************************************************************/
/*  This file is part of PPMdMem/C project, 2001                            */
/*  Designed and written by Dmitry Shkarin by request of Alexey Krivonogov  */
/*  Contents: encoding routines                                             */
/****************************************************************************/
#include "PPMdCmn.h"

#define ARI_ENC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        MPUTC(low >> 24,stream);                                            \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
#define ARI_ENCODE_SYMBOL() {                                               \
    low += LowCount*(range /= scale);       range *= HighCount-LowCount;    \
}
#define ARI_SHIFT_ENCODE_SYMBOL(SHIFT) {                                    \
    low += LowCount*(range >>= SHIFT);      range *= HighCount-LowCount;    \
}

BOOL WINAPI PPMdEncode(M_STREAM* Encoded,M_STREAM* Decoded,int MaxOrder)
{
    PPM_CONTEXT* pc;
    STATE* p, * p1;
    SEE2_CONTEXT* psee;
    WORD* pbs;
    BYTE ns;
    int i, c, Cnt;
    DWORD low=0, range=-1, LowCount, HighCount, scale;
    if ( !IS_CORRECT_INPUT(Encoded,Decoded,MaxOrder) )
                    return FALSE;

    StartModelRare(MaxOrder);
    for (ns=(pc=MaxContext)->NumStats; ; ) {
        c = MGETC(Decoded);
        if ( ns ) {
            p=pc->Stats;                    scale=pc->SummFreq;
            if (p->Symbol == c) {
                PrevSuccess=(2*(HighCount=p->Freq) > scale);
                RunLength += PrevSuccess;   LowCount=0;
                (FoundState=p)->Freq += 4;  pc->SummFreq += 4;
                if (p->Freq > MAX_FREQ)     RescaleStatistics(pc);
            } else {
                PrevSuccess=0;
                Cnt=p->Freq;                i=pc->NumStats;
                while ((++p)->Symbol != c) {
                    Cnt += p->Freq;
                    if (--i == 0) {
                        LowCount=Cnt;       CharMask[p->Symbol]=EscCount;
                        i=NumMasked=pc->NumStats;
                        do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
                        FoundState=NULL;    HighCount=scale;
                        goto SYMBOL_NOT_FOUND;
                    }
                }
                HighCount=(LowCount=Cnt)+p->Freq;
                (FoundState=p)->Freq += 4;  pc->SummFreq += 4;
                if (p[0].Freq > p[-1].Freq) {
                    _PPMD_SWAP(p[0],p[-1]); FoundState=--p;
                    if (p->Freq > MAX_FREQ) RescaleStatistics(pc);
                }
            }
SYMBOL_NOT_FOUND:
            ARI_ENCODE_SYMBOL();
        } else {
            p=ONE_STATE(pc);
            pbs=BinSumm[Freq2Indx[p->Freq-1]]+PrevSuccess+
                NS2BSIndx[pc->Suffix->NumStats]+pc->Flags+
                ((RunLength >> 26) & 0x20);
            Cnt=*pbs;
            if (p->Symbol == c) {
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
            ARI_SHIFT_ENCODE_SYMBOL(TOT_BITS);
        }
        while ( !FoundState ) {
            ARI_ENC_NORMALIZE(Encoded);
            do {
                OrderFall++;                pc=pc->Suffix;
                if ( !pc )                  goto STOP_ENCODING;
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
            p=pc->Stats-1;
            do {
                do { p++; } while (CharMask[p->Symbol] == EscCount);
                Cnt += p->Freq;
                if (p->Symbol == c) {
                    LowCount = (HighCount=Cnt)-p->Freq;
                    for (p1=p; --i ; ) {
                        do { p1++; } while (CharMask[p1->Symbol] == EscCount);
                        Cnt += p1->Freq;
                    }
                    scale += Cnt;
                    (FoundState=p)->Freq += 4;
                    pc->SummFreq += 4;
                    if (p->Freq > MAX_FREQ) RescaleStatistics(pc);
                    EscCount++;             RunLength=InitRL;
                    if (psee->Shift < PERIOD_BITS && --psee->Count == 0) {
                        psee->Summ += psee->Summ;
                        psee->Count=3 << psee->Shift++;
                    }
                    goto SYMBOL_FOUND;
                }
                CharMask[p->Symbol]=EscCount;
            } while ( --i );
            HighCount=(scale += (LowCount=Cnt));
            psee->Summ += scale;            NumMasked = pc->NumStats;
SYMBOL_FOUND:
            ARI_ENCODE_SYMBOL();
        }
        if (!OrderFall && (BYTE*) FoundState->Successor > pText)
                MaxContext=FoundState->Successor;
        else {
            UpdateModel(pc);
            if (EscCount == 0)              CLEAR_MASK(Encoded,Decoded);
        }
        ns=(pc=MaxContext)->NumStats;
        ARI_ENC_NORMALIZE(Encoded);
    }
STOP_ENCODING:
    MPUTC((low >> 24) & 0xFF,Encoded);      MPUTC((low >> 16) & 0xFF,Encoded);
    MPUTC((low >>  8) & 0xFF,Encoded);      MPUTC((low >>  0) & 0xFF,Encoded);
    PPMdPrintInfo(Decoded,Encoded);         MPutFlush(Encoded,EOS);
    return TRUE;
}
