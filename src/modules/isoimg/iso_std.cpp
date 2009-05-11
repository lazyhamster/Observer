#include "StdAfx.h"
#include "iso_std.h"

static HANDLE heap = GetProcessHeap();

#if !defined(DEBUG) && !defined(_DEBUG)

void* malloc( size_t size )
{
    assert( heap );
    return HeapAlloc( heap, HEAP_ZERO_MEMORY, size );
}

void free( void* block )
{
    assert( heap );
    if( block )
        HeapFree( heap, 0, block );
    return;
}


void* realloc( void* block, size_t size )
{
    assert( heap );

    if( block && !size )
    {
        HeapFree( heap, 0, block );
        return 0;
    }
    else if( block && size )
        return HeapReAlloc( heap, HEAP_ZERO_MEMORY, block, size );
    else if( !block && size )
        return HeapAlloc( heap, HEAP_ZERO_MEMORY, size );
    else // !block && !size
        return 0;
}

#else

void* malloc( size_t size )
{
    assert( heap );

    char* ptr = (char*)HeapAlloc( heap, HEAP_ZERO_MEMORY, size + 12 );
    assert( ptr );
    if( !ptr )
        return 0;
    *(DWORD*)ptr = 0x12345678;
    *(DWORD*)(ptr + 4) = size;
    *(DWORD*)(ptr + size + 8) = 0x12345678;

    return ptr + 8;
}

void free( void* block )
{
    assert( heap );
    if( block )
    {
        // check for boundary error
        char* ptr = (char*)block;
        if( *(DWORD*)(ptr - 8) != 0x12345678 )
            DebugString( "error before pointer" );
        DWORD size = *(DWORD*)(ptr - 4);
        if( *(DWORD*)(ptr + size) != 0x12345678 )
            DebugString( "error after pointer" );

        HeapFree( heap, 0, ptr - 8 );
    }
    return;
}

void* realloc( void* block, size_t size )
{
    assert( heap );

    if( block && !size )
    {
        free( block );
        return 0;
    }
    else if( block && size )
    {
        char* p1 = (char*)block;
        char* p2 = (char*)malloc( size );
        DWORD s1 = *(DWORD*)(p1 - 4);
        DWORD s2 = size;
        for( DWORD i = 0; i < min( s1, s2 ); i++ )
            p2[i] = p1[i];
        free( p1 );
        return p2;
    }
    else if( !block && size )
        return malloc( size );
    else // !block && !size
        return 0;
}

#endif //debug
