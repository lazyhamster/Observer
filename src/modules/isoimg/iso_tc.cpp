#include "StdAfx.h"
#include "iso_tc.h"
#include "isz/iszsdk.h"

const char ZipHeader[] = {'p', 'k'};
const char RarHeader[] = {'r', 'a', 'r'};
const char ExeHeader[] = {'M', 'Z'};

const DWORD SearchSize = 0x100000;

// NOTE: rewritten
static int lstrcmpn( const char* str1, const char* str2, int len, bool casesensitive = true )
{
    if( !len )
        return 0;

	if (casesensitive)
		return strncmp(str1, str2, len);
	else
		return _strnicmp(str1, str2, len);
}

static int lmemfind( const char* ptr1, int len1, const char* ptr2, int len2 )
{
    if( len1 < len2 )
        return -1;
    int size = len1 - len2;
    for( int i = 0; i <= size; i++ )
    {
        bool equal = true;
        for( int j = 0; j < len2; j++ )
            if( ptr1[i + j] != ptr2[j] )
            {
                equal = false;
                break;
            }
        if( equal )
            return i;
    }

    return -1;
}

static wchar_t* litoa( int num, int digits = 1 )
{
    static wchar_t buffer[100];
    int i;
    for( i = 0; i < sizeof( buffer ); i++ )
        buffer[i] = '0';
    for( i = 0; num > 0; i++, num /= 10 )
        buffer[sizeof( buffer ) - 1 - i] = (wchar_t)((num % 10) + '0');
    return buffer + sizeof( buffer ) - max( i, digits );
}

static DWORD ReadRawDataByPos( HANDLE file, LONGLONG position, DWORD size, void* data )
{
	assert( file != 0 && file != INVALID_HANDLE_VALUE );
	assert( data );

	LONG low = LOWDWORD( position );
	LONG high = HIDWORD( position );

	if( ((LONGLONG)SetFilePointer( file, low, &high, FILE_BEGIN ) + ((LONGLONG)high << 32)) != position )
		return 0;

	DWORD read;

	if( ReadFile( file, data, size, &read, 0 ) )
		return read;
	else
		return 0;
}

static DWORD ReadIszDataByPos( HANDLE file, LONGLONG position, DWORD size, void* data )
{
	unsigned int sec_size = 0;
	unsigned int secs_count = isz_get_capacity(file, &sec_size);

	unsigned int start_sec = (unsigned int) (position / sec_size);
	unsigned int read_sec_count = size / sec_size + 1;
	if (start_sec >= secs_count || read_sec_count == 0)
		return 0;

	int bufDataOffset = (int) (position - start_sec * sec_size);
	size_t bufReadSize = read_sec_count * sec_size;
	// If we are reading at the start of the sector and reading amount of bytes aligned to sector size
	// then do not allocate additional buffer
	if (bufDataOffset == 0 && bufReadSize == size)
	{
		return isz_read_secs(file, data, start_sec, read_sec_count);
	}
	else
	{
		void* tmpBuf = malloc(bufReadSize);
		DWORD readVal = isz_read_secs(file, tmpBuf, start_sec, read_sec_count);
		
		DWORD copySize = min(size, readVal);
		memcpy(data, (char*)tmpBuf + bufDataOffset, copySize);
		
		free(tmpBuf);
		return copySize;
	}
}

static DWORD ReadDataByPos( const IsoImage* image, LONGLONG position, DWORD size, void* data )
{
    assert( image && size && data );
	
	switch (image->ImageType)
	{
		case ISOTYPE_RAW:
			return ReadRawDataByPos(image->hFile, position, size, data);
		case ISOTYPE_ISZ:
			return ReadIszDataByPos(image->hFile, position, size, data);
	}

	return 0;
}

static LONGLONG GetBlockOffset( const IsoImage* image, DWORD block )
{
    return (LONGLONG)(DWORD)block * (WORD)image->RealBlockSize + (image->HeaderSize ? image->HeaderSize : image->DataOffset);
}

DWORD ReadBlock( const IsoImage* image, DWORD block, DWORD size, void* data )
{
    return ReadDataByPos( image, GetBlockOffset( image, block ), size, data );
}

static int ScanBootSections( const IsoImage* image, LONGLONG offset, SectionHeaderEntry* firstHeader, SectionEntry* entries )
{
    assert( image );
    assert( firstHeader );

    int images = 0;

    SectionHeaderEntry header = *firstHeader;
    do
    {
        CatalogEntry entry;

        for( int i = 0; i < header.NumberOfSectionEntries; i++, offset += sizeof( entry ) )
        {
            if( ReadDataByPos( image, offset, sizeof( entry ), &entry ) )
            {
                if( entry.Entry.BootIndicator == 0x88 )
                {
                    if( entries )
                        entries[images] = entry.Entry;
                    images++;
                }

                if( entry.Entry.BootMediaType & (1 << 5) )
                { // next entry is extension
                    for( ; ReadDataByPos( image, offset, sizeof( entry ), &entry ) &&
                           entry.Extension.ExtensionIndicator == 0x44                     &&
                           entry.Extension.Bits & (5 << 1); offset += sizeof( entry ) );
                }
            }
            else
            {
                DebugString( "can't read SectionEntry" );
                return 0;
            }
        }
    } while( header.HeaderIndicator == 90 );
    return images;
}

// if sharp == 1 then we should make only 1 iteration in loop
static DWORD GetVolumeDescriptor( const IsoImage* image, PrimaryVolumeDescriptorEx* descriptor,
                                  DWORD startblk = 0, bool sharp = false )
{
    assert( descriptor );
    assert( image );

    ZeroMemory( descriptor, sizeof( descriptor ) );

    bool step1 = false;

	DWORD iterations = 0;
	DWORD max_iterations = 1024 * 1024 * 100 / image->RealBlockSize; // max 100 Mb between sessions

    for( DWORD i = startblk;
         !step1 && iterations < max_iterations &&
         ReadBlock( image, i, sizeof( descriptor->VolumeDescriptor ),
                    &descriptor->VolumeDescriptor ) == sizeof( descriptor->VolumeDescriptor );
         i++, step1 = sharp, iterations++ )
    {
        if( !lstrcmpn( (char*)descriptor->VolumeDescriptor.StandardIdentifier, CDSignature, sizeof(CDSignature)) &&
            descriptor->VolumeDescriptor.VolumeDescriptorType == 1 )
        {
            DWORD block = i;
            // Found it!
            DebugString("Found it");
			iterations = 0;
            // trying to read root directory
            char* buffer = (char*)malloc( (WORD)descriptor->VolumeDescriptor.LogicalBlockSize );
            if( buffer )
            {
                if( ReadBlock( image, (DWORD)descriptor->VolumeDescriptor.DirectoryRecordForRootDirectory.LocationOfExtent,
                               (WORD)descriptor->VolumeDescriptor.LogicalBlockSize, buffer ) !=
                               (WORD)descriptor->VolumeDescriptor.LogicalBlockSize )
                {
                    free( buffer );
                    continue;
                }
                // ok, we can read root directory...
                free( buffer );
            }

            for( ;!descriptor->Unicode && i < (DWORD)descriptor->VolumeDescriptor.VolumeSpaceSize; i++ )
            {
                PrimaryVolumeDescriptor unicode;
                ZeroMemory( &unicode, sizeof( unicode ) );
                if( ReadBlock( image, i, sizeof( unicode ), &unicode ) != sizeof( unicode ) ||
                    lstrcmpn( (char*)unicode.StandardIdentifier, CDSignature, sizeof( CDSignature ) ) )
                    break;

                if( unicode.VolumeDescriptorType == 2                               &&
                    (DWORD)unicode.DirectoryRecordForRootDirectory.LocationOfExtent &&
                    (DWORD)unicode.DirectoryRecordForRootDirectory.DataLength )
                {
                    descriptor->Unicode = true;
                    descriptor->VolumeDescriptor = unicode;
                    DebugString("image.Unicode = TRUE");
                    break;
                }
            }

            assert( sizeof( BootRecordVolumeDescriptor ) == 0x800 );
            assert( sizeof( InitialEntry ) == 0x20 );
            assert( sizeof( SectionHeaderEntry ) == 0x20 );
            assert( sizeof( SectionEntry ) == 0x20 );
            assert( sizeof( SectionEntryExtension ) == 0x20 );
    
            BootRecordVolumeDescriptor boot;
            if( ReadBlock( image, block + 1, sizeof( boot ), &boot ) )
            { // check for boot image
                if( !boot.BootRecordIndicator &&
                    !lstrcmpn( (char*)boot.StandardIdentifier, CDSignature, sizeof( CDSignature ) ) &&
                    boot.VersionOfDescriptor == 1 &&
                    !lstrcmpn( (char*)boot.BootSystemIdentifier, TORITO, sizeof( TORITO ) ) )
                {
                    BootCatalog catalog;
                    if( ReadBlock( image, boot.BootCatalogPointer, sizeof( catalog ), &catalog ) )
                    {
                        DebugString( "reading boot catalog" );
                        if( (unsigned char)catalog.Validation.HeaderID == (unsigned char)1 &&
                            (short unsigned int)catalog.Validation.KeyWord == (short unsigned int)0xaa55 )
                        {
                            unsigned short sum = 0;
                            for( int j = 0; j < sizeof( catalog.Validation ) / sizeof( catalog.Validation.Checksum ); j++ )
                                sum = (unsigned short)(sum + *((unsigned short*)(&catalog.Validation) + j));
                            if( sum )
                            {
                                assert( "checksum is wrong :(" );
                            }
                            else
                            {
                                DebugString( "boot catalog detected" );
                                LONGLONG offset = GetBlockOffset( image, boot.BootCatalogPointer ) + sizeof( catalog );
                                int images = ScanBootSections( image, offset, &catalog.Entry[1].Header, 0 );
                                if( catalog.Entry[0].Initial.Bootable == 0x88 )
                                    images++;
                                if( images )
                                { // reading images
                                    descriptor->BootCatalog = (BootCatalog*)malloc( sizeof( BootCatalog ) +
                                                        max( 0, images - 2 ) * sizeof( CatalogEntry ) );
                                    assert( descriptor->BootCatalog );
                                    if( catalog.Entry[0].Initial.Bootable == 0x88 )
                                    {
                                        descriptor->BootCatalog->Entry[0] = catalog.Entry[0];
                                        ScanBootSections( image, offset, &catalog.Entry[1].Header,
                                                          &descriptor->BootCatalog->Entry[1].Entry );
                                    }
                                    else
                                        ScanBootSections( image, offset, &catalog.Entry[1].Header,
                                                          &descriptor->BootCatalog->Entry[0].Entry );
                                    descriptor->BootImageEntries = (DWORD)images;
                                }
                            }
                        }
                        else
                            DebugString( "wrong validation signature" );
                    }
                    else
                        DebugString( "can't read boot catalog from file" );
                }
            }

            return block;
        }
        else if(!lstrcmpn( (char*)descriptor->XBOXVolumeDescriptor.StandardIdentifier,
                           XBOXSignature, sizeof(XBOXSignature)) &&
                !lstrcmpn( (char*)descriptor->XBOXVolumeDescriptor.StandardIdentifier2,
                           XBOXSignature, sizeof(XBOXSignature)))
        {
            descriptor->XBOX = true;
            return i;
        }
    }

    return 0;
}

static void CloseIsoHandle( IsoImage* image )
{
	assert( image );

	if( image->hFile && image->hFile != INVALID_HANDLE_VALUE )
	{
		switch (image->ImageType)
		{
		case ISOTYPE_ISZ:
			isz_close(image->hFile);
			break;
		default:
			CloseHandle( image->hFile );
			break;
		}
		
		image->hFile = INVALID_HANDLE_VALUE;
	}
}

IsoImage* GetImage( const wchar_t* filename, const char* passwd, bool &needPasswd )
{
    DebugString( "GetImage" );
    needPasswd = false;
	if( !filename )
        return NULL;

    IsoImage image;
    ZeroMemory( &image, sizeof( image ) );

	HANDLE hImgHandle = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	if (hImgHandle == INVALID_HANDLE_VALUE) return NULL;

	HANDLE hIszHandle = isz_open(hImgHandle, filename);
	if (hIszHandle != INVALID_HANDLE_VALUE)
	{
		image.ImageType = ISOTYPE_ISZ;
		image.hFile = hIszHandle;
	}
	else
	{
		image.ImageType = ISOTYPE_RAW;
		image.hFile = hImgHandle;
	}

	//Check for password protection
	if (image.ImageType == ISOTYPE_ISZ && isz_needpassword(image.hFile))
	{
		if (!passwd || !*passwd)
		{
			needPasswd = true;
			return NULL;
		}
		else
		{
			isz_setpassword(image.hFile, passwd);
		}
	}

    assert( sizeof( PrimaryVolumeDescriptor ) == 0x800 ); // check for size of descriptor
    DWORD read;
    
    // check for zip or rar archives
    char ArcHeaderBuf[0x20];
    if( ReadDataByPos( &image, 0, sizeof( ArcHeaderBuf ), ArcHeaderBuf ) != sizeof( ArcHeaderBuf ) ||
        !lstrcmpn( ExeHeader, ArcHeaderBuf, sizeof( ExeHeader ), false )                           ||
        !lstrcmpn( ZipHeader, ArcHeaderBuf, sizeof( ZipHeader ), false )                           ||
        !lstrcmpn( RarHeader, ArcHeaderBuf, sizeof( RarHeader ), false ) )
    {
        DebugString( "hmmm, this image can't be read or it has zip or rar signature..." );
        CloseIsoHandle(&image);
        return NULL;
    }
    
    PrimaryVolumeDescriptor descriptor;
    ZeroMemory( &descriptor, sizeof( descriptor ) );

    // Read the VolumeDescriptor (2k) until we find it, but only up to to 1MB
    for( ; image.DataOffset < SearchSize + sizeof( PrimaryVolumeDescriptor ); image.DataOffset += sizeof( PrimaryVolumeDescriptor ) )
    {
        char buffer[sizeof( PrimaryVolumeDescriptor ) + sizeof( CDSignature )];
        if( ReadDataByPos( &image, 0x8000 + image.DataOffset,
                           sizeof( buffer ), buffer ) != sizeof( buffer ) )
        {
            // Something went wrong, probably EOF?
            DebugString("Could not read complete VolumeDescriptor block");
            CloseIsoHandle(&image);
            return 0;
        }

        int sig_offset = lmemfind( buffer, sizeof( buffer ), CDSignature, sizeof( CDSignature ) );
        if( sig_offset >= 0 )
            image.DataOffset += sig_offset - 1;
        else
        {
            if( image.DataOffset >= SearchSize )
            {
                // Just to make sure we don't read in a too big file, stop after 1MB.
                DebugString("Reached 1MB without descriptor");
                CloseIsoHandle(&image);
                return 0;
            }
            //image.DataOffset += sizeof( image.VolumeDescriptor );
            continue;
        }

        // Try to read a block
        read = ReadDataByPos( &image, 0x8000 + image.DataOffset, sizeof( descriptor ), &descriptor );
        
        if( read != sizeof( descriptor ) )
        {
            // Something went wrong, probably EOF?
            DebugString("Could not read complete VolumeDescriptor block");
            CloseIsoHandle(&image);
            return 0;
        }

        if( lstrcmpn( (char*)descriptor.StandardIdentifier, CDSignature, sizeof( CDSignature ) ) == 0 &&
            descriptor.VolumeDescriptorType == 1 )
        {
            // Found it!
            DebugString("Found it");
            break;
        }
    }

    if( image.DataOffset >= SearchSize )
    {
        // Just to make sure we don't read in a too big file.
        DebugString("Reached 1MB without descriptor");
        CloseIsoHandle(&image);
        return 0;
    }

    image.RealBlockSize = (WORD)descriptor.LogicalBlockSize;

    // detect for next signature CDOO1
    char buffer[10000];
    if( ReadDataByPos( &image, 0x8000 + image.DataOffset + 6, sizeof( buffer ), buffer ) )
    {
        int pos = lmemfind( buffer, sizeof( buffer ), CDSignature, sizeof( CDSignature ) );
        if( pos >= 0 )
            image.RealBlockSize = pos + sizeof( CDSignature );
        image.HeaderSize = (0x8000 + image.DataOffset) % image.RealBlockSize;
    }

    // check for strange nero format
    if( image.DataOffset + 0x8000 == image.RealBlockSize * 0xa6 + image.HeaderSize )
        image.HeaderSize += image.RealBlockSize * 0x96;
    else if( image.HeaderSize > 0xff )
        image.HeaderSize = 0;

    PrimaryVolumeDescriptorEx descriptorex;
    ZeroMemory( &descriptorex, sizeof( descriptorex ) );
    DWORD fblock = 0;

    for( image.DescriptorNum = 0;
         (fblock = GetVolumeDescriptor( &image, &descriptorex, fblock )) != 0;
         image.DescriptorNum++ )
    {
        image.VolumeDescriptors = (PrimaryVolumeDescriptorEx*)realloc( image.VolumeDescriptors,
            (image.DescriptorNum + 1) * sizeof( *image.VolumeDescriptors ) );
        image.VolumeDescriptors[image.DescriptorNum] = descriptorex;

        // checking for XBOX image inside of current ISO image
        DWORD xbox = fblock + 0x10;
        fblock += (DWORD)descriptorex.VolumeDescriptor.VolumeSpaceSize;
        xbox = GetVolumeDescriptor(&image, &descriptorex, xbox, true);
        if(xbox)
        {
            image.VolumeDescriptors = (PrimaryVolumeDescriptorEx*)realloc( image.VolumeDescriptors,
                (image.DescriptorNum + 2) * sizeof( *image.VolumeDescriptors ) );
            image.VolumeDescriptors[image.DescriptorNum + 1] = descriptorex;
            image.DescriptorNum++;
        }

        ZeroMemory( &descriptorex, sizeof( descriptorex ) );
    }

	if( image.VolumeDescriptors == NULL )
	{
		DebugString("Malformed ISO file. No VolumeDescriptors found.");
		CloseIsoHandle(&image);
		return NULL;
	}

    IsoImage* pimage = (IsoImage*)malloc( sizeof( *pimage ) );
    assert( pimage );
    *pimage = image;

    return pimage;
}

bool FreeImage( IsoImage* image )
{
    DebugString( "FreeImage" );
    assert( image );

    CloseIsoHandle(image);

    if( image->DirectoryList )
    {
        for(DWORD i = 0; i < image->DirectoryCount; i++)
        {
            if(image->DirectoryList[i].FileName)
                free(image->DirectoryList[i].FileName);
            if(image->DirectoryList[i].FilePath)
                free(image->DirectoryList[i].FilePath);
        }
        free( image->DirectoryList );
    }

    for( DWORD i = 0; i < image->DescriptorNum && image->VolumeDescriptors; i++ )
        if( image->VolumeDescriptors[i].BootCatalog )
            free( image->VolumeDescriptors[i].BootCatalog );
    free( image->VolumeDescriptors );
    
    free( image );

    return true;
}

static bool LoadXBOXTree(IsoImage* image, PrimaryVolumeDescriptorEx* desc, const wchar_t* path,
                         unsigned int block, unsigned int size, Directory** dirs, DWORD* count)
{
    if(!block || !count || !size)
        return true;

    if( size > 10 * 1024 * 1024 )
        return false;

    char* data = (char*)malloc( size );
    assert( data );
    if( !data )
        return false;

    unsigned short* stack = (unsigned short*)malloc(size);
    int stackptr = 0;
    assert(stack);
    if( !stack )
    {
        free(data);
        return false;
    }

    if( ReadBlock(image, block, size, data) != size )
    {
        free(data);
        free(stack);
        return false;
    }

    DWORD offset = 0;
    bool result = true;

    do
    {
        XBOXDirectoryRecord* record = (XBOXDirectoryRecord*)(data + offset);
        Directory directory;
        ZeroMemory( &directory, sizeof( directory ) );
        directory.XBOXRecord = *record;
        directory.VolumeDescriptor = desc;

		if(dirs)
        { // if dirs == 0 - we just calculating total number of records...
            directory.FileName = (wchar_t*) malloc((record->LengthOfFileIdentifier + 2) * sizeof(*directory.FileName));
		    assert(directory.FileName);
            if(!directory.FileName)
            {
                free(data);
                free(stack);
                return false;
            }
            MultiByteToWideChar(CP_UTF8, 0, (char*)record->FileIdentifier, record->LengthOfFileIdentifier, directory.FileName, record->LengthOfFileIdentifier + 2);
            directory.FileName[record->LengthOfFileIdentifier] = 0;

		    int dirlen = lstrlen(path) + 1 + lstrlen(directory.FileName) + 1;
		    directory.FilePath = (wchar_t*) malloc(dirlen * sizeof(*directory.FilePath));
		    assert(directory.FilePath);
            if(!directory.FilePath)
            {
                free(data);
                free(stack);
                return false;
            }
            lstrcat( lstrcat( lstrcpy( directory.FilePath, path ), path[0] ? L"\\" : L"" ), directory.FileName );

            (*dirs)[*count] = directory;
        }

        (*count)++;
        if( directory.XBOXRecord.FileFlags & XBOX_DIRECTORY )
            if( !LoadXBOXTree( image, desc, directory.FilePath,
                               directory.XBOXRecord.LocationOfExtent, directory.XBOXRecord.DataLength,
                               dirs, count ) )
                result = false;

        if(directory.XBOXRecord.RightSubTree) // push right ptr to stack
            stack[stackptr++] = directory.XBOXRecord.RightSubTree;

        offset = directory.XBOXRecord.LeftSubTree * 4;
        if(!offset && stackptr) // if we got last element in "line" - then get last element from stack
            offset = stack[--stackptr] * 4;
    } while(offset);

    free(stack);
    free(data);

    return result;
}

static SystemUseEntryHeader* GetSystemUseEntry(const char *entrySig, int entryVer, const DirectoryRecord *record)
{
	int nSysUseStart = sizeof(DirectoryRecord) + record->LengthOfFileIdentifier - 1;
	nSysUseStart += (nSysUseStart & 1); // Align to even number of bytes

	if (record->LengthOfDirectoryRecord > nSysUseStart)
	{
		int nPos = nSysUseStart;
		while (nPos + 4 <= record->LengthOfDirectoryRecord)
		{
			SystemUseEntryHeader* suh = (SystemUseEntryHeader*) ((char*) record + nPos);
			if (suh->Length <= 0) break;

			if (suh->Signature[0] == entrySig[0] && suh->Signature[1] == entrySig[1] && suh->Version == entryVer)
			{
				return suh;
			}
			nPos += suh->Length;
		} // while
	}
	
	return NULL;
}

static bool LoadTree( IsoImage* image, PrimaryVolumeDescriptorEx* desc, const wchar_t* path, DirectoryRecord* root,
                      Directory** dirs, DWORD* count, bool boot = false )
{
    assert( image && root && count && desc );

    if(desc->XBOX)
        return LoadXBOXTree(image, desc, path, desc->XBOXVolumeDescriptor.LocationOfExtent,
                            desc->XBOXVolumeDescriptor.DataLength, dirs, count);

    //DebugString( "LoadTree" );
    DWORD block = (DWORD)root->LocationOfExtent;
    DWORD size = (DWORD)root->DataLength;

    if( !block || !count )
        return true;

    if( size > 10 * 1024 * 1024 )
        return false;

    char* data = (char*)malloc( size );
    assert( data );
    if( !data )
        return false;

    DWORD position = 0;
    for( DWORD k = 0; position < size; position += (WORD)desc->VolumeDescriptor.LogicalBlockSize, k++ )
    {
        DWORD s = min( size - position, (WORD)desc->VolumeDescriptor.LogicalBlockSize );
        if( ReadBlock( image, block + k, s, data + position ) != s )
        {
            free( data );
            return false;
        }
    }
    
    //if( ReadBlock( image, block, size, data ) != size )
    //{
    //    free( data );
    //    return false;
    //}

    bool result = true;
    
    DWORD offset = 0;
    DWORD num = 0;
    while( offset < size - 0x21 )
    {
        while( !data[offset] && offset < size - 0x21 )
            offset++;

        if( offset >= size - 0x21 )
            break;

        DirectoryRecord* record = (DirectoryRecord*)(data + offset);
        Directory directory;
        ZeroMemory( &directory, sizeof( directory ) );
        directory.Record = *record;
        directory.VolumeDescriptor = desc;

        if( (((int)record->LengthOfDirectoryRecord + 1) & ~1) <
            ((((int)sizeof( *record ) - (int)sizeof( *record->FileIdentifier ) + (int)record->LengthOfFileIdentifier) + 1) & ~1) )
            break;

        if(dirs)
        { // if dirs == 0 - we just calculating total number of records...
            if( desc->Unicode && record->LengthOfFileIdentifier > 1 && num > 1 )
            {
                UINT codepage = CP_UTF8;
                int i;
                int length = record->LengthOfFileIdentifier;
            
                for( i = 0; i < record->LengthOfFileIdentifier; i++ )
                {
                    if( record->FileIdentifier[i] < 32 )
                    {
                        codepage = GetACP();
                        length = record->LengthOfFileIdentifier / 2;
                        WCHAR* uname = (WCHAR*)record->FileIdentifier;
                        for( i = 0; i < (record->LengthOfFileIdentifier / 2); i++ ) // swap bytes in name
                            uname[i] = (WORD)((uname[i] >> 8) + (uname[i] << 8));

					    directory.FileName = (wchar_t*) malloc((length + 1) * sizeof(*directory.FileName));
					    assert(directory.FileName);
                        if(!directory.FileName)
                        {
                            free(data);
                            return false;
                        }

                        lstrcpyn(directory.FileName, (WCHAR*)record->FileIdentifier, length + 1);
                        directory.FileName[length] = 0;

                        break;
                    }
                }

                if( codepage == CP_UTF8 )
			    {
				    directory.FileName = (wchar_t*) malloc((record->LengthOfFileIdentifier + 2) * sizeof(*directory.FileName));
				    assert(directory.FileName);
                    if(!directory.FileName)
                    {
                        free(data);
                        return false;
                    }
                    MultiByteToWideChar(CP_UTF8, 0, (char*)record->FileIdentifier, record->LengthOfFileIdentifier, directory.FileName, record->LengthOfFileIdentifier + 2);
                    directory.FileName[record->LengthOfFileIdentifier] = 0;
			    }
            
                if( wcsrchr( directory.FileName, ';' ) )
                    *wcsrchr( directory.FileName, ';' ) = 0;

			    int dirlen = lstrlen(path) + 1 + lstrlen(directory.FileName) + 1;
			    directory.FilePath = (wchar_t*)malloc(dirlen * sizeof(*directory.FilePath));
			    assert(directory.FilePath);
                if(!directory.FilePath)
                {
                    free(data);
                    return false;
                }
                lstrcat( lstrcat( lstrcpy( directory.FilePath, path ), (path[0]) ? L"\\" : L"" ), directory.FileName );

                (*dirs)[*count] = directory;
            }
            else if( !desc->Unicode && record->LengthOfFileIdentifier > 0 && num > 1 )
            {
			    // Search for NM record (RockRidge extension)
				bool fNMRecFound = false;
				if (desc->SystemUseAreas)
				{
					SystemUseEntryHeader* suh = GetSystemUseEntry("NM", 1, record);
					if (suh)
					{
						size_t nNameBufLen = (suh->Length - 4);
						directory.FileName = (wchar_t*) malloc((nNameBufLen + 1) * sizeof(wchar_t));
						assert(directory.FileName);
						if(!directory.FileName)
						{
							free(data);
							return false;
						}
						MultiByteToWideChar(GetACP(), 0, (char*)suh + 5, (int) nNameBufLen - 1, directory.FileName, (int) nNameBufLen + 1);
						directory.FileName[nNameBufLen] = 0;
							
						fNMRecFound = true;
					}
				}

				// If extended name not found or not searched for use standard one
				if (!fNMRecFound)
				{
					directory.FileName = (wchar_t*) malloc((record->LengthOfFileIdentifier + 2) * sizeof(*directory.FileName));
					assert(directory.FileName);
					if(!directory.FileName)
					{
	                    free(data);
		                return false;
			        }
				    MultiByteToWideChar(GetACP(), 0, (char*)record->FileIdentifier, record->LengthOfFileIdentifier, directory.FileName, record->LengthOfFileIdentifier + 2);
					directory.FileName[record->LengthOfFileIdentifier] = 0;
				}

                if( wcsrchr( directory.FileName, ';' ) )
                    *wcsrchr( directory.FileName, ';' ) = 0;

			    int dirlen = lstrlen(path) + 1 + lstrlen(directory.FileName) + 1;
			    directory.FilePath = (wchar_t*) malloc(dirlen * sizeof(*directory.FilePath));
			    assert(directory.FilePath);
                if(!directory.FilePath)
                {
                    free(data);
                    return false;
                }
                lstrcat( lstrcat( lstrcpy( directory.FilePath, path ), path[0] ? L"\\" : L"" ), directory.FileName );

                (*dirs)[*count] = directory;
            }

			// Check for System Use Entries
			if (!desc->Unicode && (record->LengthOfFileIdentifier == 1) && (*count == 0) && (!path || !*path) && !num)
			{
				SystemUseEntryHeader* suh = GetSystemUseEntry("SP", 1, record);
				if (suh)
				{
					unsigned char* rawSuh = (unsigned char*) suh + sizeof(SystemUseEntryHeader);
					if (suh->Length == 7 && rawSuh[0] == 0xBE && rawSuh[1] == 0xEF)
						desc->SystemUseAreas = true;
				}
			}
        } //if dirs

        if((!desc->Unicode && record->LengthOfFileIdentifier > 0 ||
             desc->Unicode && record->LengthOfFileIdentifier > 1) && num > 1)
        {
            (*count)++;
            if( directory.Record.FileFlags & FATTR_DIRECTORY )
                if( !LoadTree( image, desc, directory.FilePath, &directory.Record, dirs, count ) )
                    result = false;
        }

        offset += record->LengthOfDirectoryRecord;
        num++;
    }  //while

    if( desc->BootImageEntries && boot )
    {
        if( dirs )
        {
            Directory* dir = (*dirs) + (*count);
            ZeroMemory( dir, sizeof( *dir ) );
            dir->VolumeDescriptor = desc;

            const wchar_t* boot_images = L"boot.images";
            size_t len = (lstrlen(path) + 1 + lstrlen(boot_images) + 1) * sizeof(wchar_t);
            dir->FileName = (wchar_t*)malloc(len);
            dir->FilePath = (wchar_t*)malloc(len);

            if(!dir->FileName || !dir->FilePath)
            {
                free(data);
                return false;
            }

			memset(dir->FileName, 0, len);
			memset(dir->FilePath, 0, len);

            if( lstrlen( path ) )
            {
                lstrcpy( dir->FilePath, lstrcpy( dir->FileName, path ) );
                lstrcat( dir->FilePath, L"\\" );
                lstrcat( dir->FileName, L"\\" );
            }
            
            lstrcat( dir->FilePath, boot_images );
            lstrcat( dir->FileName, boot_images );
            dir->Record.FileFlags = FATTR_DIRECTORY | FATTR_HIDDEN;
        }
        (*count)++;
    }

    for( DWORD i = 0; i < desc->BootImageEntries && boot; i++ )
    {
        DebugString( "reading boot images" );
        if( dirs )
        {
            Directory* dir = (*dirs) + (*count);
            ZeroMemory( dir, sizeof( *dir ) );
            CatalogEntry* BootEntry = &desc->BootCatalog->Entry[i];
            wchar_t number[4] = L"00";
            number[1] = (wchar_t)((i % 10) + '0');
            number[0] = (wchar_t)(((i / 10) % 10) + '0');
            
            wchar_t mediaType[50];
            const wchar_t* floppy = L"floppy";
            const wchar_t* harddisk  = L"harddisk";
            const wchar_t* no_emul = L"no_emul";
            const wchar_t* img = L".";
            const wchar_t* ima = L".ima";
            const wchar_t* _1_20_ = L"_1.20.";
            const wchar_t* _1_44_ = L"_1.44.";
            const wchar_t* _2_88_ = L"_2.88.";
            //const char* img = ".img";

            switch( BootEntry->Entry.BootMediaType )
            {
            case 0:
                dir->FileName = (wchar_t*)malloc((lstrlen(no_emul) + lstrlen(img) + lstrlen(number) + 1) * sizeof(*dir->FileName));
                assert(dir->FileName);
                lstrcat( lstrcat( lstrcpy( dir->FileName, no_emul ), img ), number);
                dir->Record.DataLength = BootEntry->Entry.SectorCount * 0x200;
                break;
            case 1:
                dir->FileName = (wchar_t*)malloc((lstrlen(floppy) + lstrlen(_1_20_) + lstrlen(number) + lstrlen(ima) + 1) * sizeof(*dir->FileName));
                assert(dir->FileName);
                lstrcat( lstrcat( lstrcat( lstrcpy( dir->FileName, floppy ), _1_20_), number ), ima );
                dir->Record.DataLength = 0x50 * 0x2 * 0x0f * 0x200;
                break;
            case 2:
                dir->FileName = (wchar_t*)malloc((lstrlen(floppy) + lstrlen(_1_44_) + lstrlen(number) + lstrlen(ima) + 1) * sizeof(*dir->FileName));
                assert(dir->FileName);
                lstrcat( lstrcat( lstrcat( lstrcpy( dir->FileName, floppy ), _1_44_), number ), ima );
                dir->Record.DataLength = 0x50 * 0x2 * 0x12 * 0x200;
                break;
            case 3:
                dir->FileName = (wchar_t*)malloc((lstrlen(floppy) + lstrlen(_2_88_) + lstrlen(number) + lstrlen(ima) + 1) * sizeof(*dir->FileName));
                assert(dir->FileName);
                lstrcat( lstrcat( lstrcat( lstrcpy( dir->FileName, floppy ), _2_88_), number ), ima );
                dir->Record.DataLength = 0x50 * 0x2 * 0x24 * 0x200;
                break;
            case 4:
                {
                    dir->FileName = (wchar_t*)malloc((lstrlen(harddisk) + lstrlen(img) + lstrlen(number) + 1) * sizeof(*dir->FileName));
                    assert(dir->FileName);
                    lstrcat( lstrcat( lstrcpy( dir->FileName, harddisk ), img ), number );
                    MBR mbr;
                    if( ReadBlock( image, BootEntry->Entry.LoadRBA, 0x200, &mbr ) )
                        if( mbr.Signature == (unsigned short)0xaa55 )
						{
                            dir->Record.DataLength = (mbr.Partition[0].start_sect + mbr.Partition[0].nr_sects) * 0x200;
							dir->Record.FileFlags |= FATTR_ALLOWPARTIAL;
						}
                        else
                        {
                            DebugString( "hard disk signature doesn't match" );
                            dir->Record.DataLength = BootEntry->Entry.SectorCount * 0x200;
                        }
                    else
                    {
                        DebugString( "can't read MBR for hard disk emulation" );
                        dir->Record.DataLength = BootEntry->Entry.SectorCount * 0x200;
                    }
                }
                break;
            default:
                lstrcpy( mediaType, L".unknown" );
                dir->Record.DataLength = BootEntry->Entry.SectorCount * 0x200;
                break;
            }  //switch
            
            int dirlen = lstrlen( path ) + lstrlen(L"\\boot.images\\") + lstrlen(dir->FileName) + 1;
            dir->FilePath = (wchar_t*)malloc(dirlen * sizeof(*dir->FilePath));
            
            if( lstrlen( path ) )
                lstrcat( lstrcat( lstrcpy( dir->FilePath, path ), L"\\boot.images\\" ), dir->FileName );
            else
                lstrcat( lstrcpy( dir->FilePath, L"boot.images\\" ), dir->FileName );

            dir->Record.LocationOfExtent = BootEntry->Entry.LoadRBA;
            dir->VolumeDescriptor = desc;
        } //if dirs
        (*count)++;
    }  //for

    free( data );

    return result;
}

bool LoadAllTrees( IsoImage* image, Directory** dirs, DWORD* count, bool boot )
{
    if( image->DescriptorNum < 2 )
        return LoadTree( image, image->VolumeDescriptors, L"",
                         &image->VolumeDescriptors->VolumeDescriptor.DirectoryRecordForRootDirectory,
                         dirs, count, boot );
    else
    {
        bool result = false;
        int digs;
        int descnum = image->DescriptorNum;
        for( digs = 0; descnum; digs++, descnum /= 10 );
        for( DWORD i = 0; i < image->DescriptorNum; i++ )
        {
            wchar_t session[50] = L"";
            if(!image->VolumeDescriptors[i].XBOX)
            {
                lstrcpy(session, L"session");
                lstrcat( session, litoa( i + 1, digs ) );
            }
            //else
            //    lstrcpy(session, "XBOX");
            result |= LoadTree( image, image->VolumeDescriptors + i, session,
                                &image->VolumeDescriptors[i].VolumeDescriptor.DirectoryRecordForRootDirectory,
                                dirs, count, boot );
        }
        return result;
    }
}
