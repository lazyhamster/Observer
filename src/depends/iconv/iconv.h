/* This is a repackaging of win_iconv by Yukihiro Nakadaira. The
 * actual source code in win_iconv.c is by him. I just added some
 * small bits: this trivial iconv.h header, the win_iconv_dll.c
 * wrapper that makes it possible to build win_iconv.c into a DLL that
 * has the same API and ABI as GNU libiconv, the Makefile.
 *
 * --Tor Lillqvist <tml@iki.fi>, January 2008
 */

#ifndef ICONV_H_INCLUDED
#define ICONV_H_INCLUDED 1

#include <stdlib.h>		/* For size_t */

#ifdef __cplusplus
extern "C" {
#endif
typedef void* iconv_t;
iconv_t iconv_open(const char *tocode, const char *fromcode);
int iconv_close(iconv_t cd);
size_t iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
#ifdef __cplusplus
}
#endif
#endif
