#ifndef _ISO_TC_H_
#define _ISO_TC_H_

#include "iso.h"
#include "iso_std.h"

#define LOWDWORD(x) (DWORD)(x)
#define HIDWORD(x) (DWORD)((x) >> 32)

IsoImage* GetImage( const wchar_t* filename );
bool FreeImage( IsoImage* image );
bool LoadAllTrees( IsoImage* image, Directory** dirs, DWORD* count, bool boot = false );

DWORD ReadDataByPos( HANDLE file, LONGLONG position, DWORD size, void* data );
LONGLONG GetBlockOffset( const IsoImage* image, DWORD block );
DWORD ReadBlock( const IsoImage* image, DWORD block, DWORD size, void* data );

#endif //_ISO_TC_H_
