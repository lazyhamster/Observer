#ifndef ppmd_def_h__
#define ppmd_def_h__

#include "gea/PPMD/endeppmd.h"

#ifdef  __cplusplus
extern "C" {
#endif

dword STDCALL ppmd_start( dword memory );
void STDCALL ppmd_stop( void );

dword STDCALL ppmd_decode( pbyte in, dword size, pbyte out, dword outsize, psppmd ppmd );

#ifdef  __cplusplus
}
#endif

#endif // ppmd_def_h__
