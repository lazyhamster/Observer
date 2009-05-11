#ifndef _ISO_STD_H_
#define _ISO_STD_H_

#if defined(DEBUG) || defined(_DEBUG)
#define DebugString(x) OutputDebugStringA( __STR__(__LINE__)": " ),OutputDebugStringA( x ),OutputDebugStringA( "\n" )
#define __STR2__(x) #x
#define __STR__(x) __STR2__(x)
#else
#define DebugString(x)
#endif

void* malloc( size_t size );
void free( void* block );
void* realloc( void* block, size_t size );

#endif //_ISO_STD_H_