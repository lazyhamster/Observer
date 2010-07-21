#include "StdAfx.h"
#include "ModuleDef.h"
#include "iso_tc.h"

#define NAME_PATH_BUFSIZE 2048

DWORD GetDirectoryAttributes(Directory* dir);

FILETIME VolumeDateTimeToFileTime(VolumeDateTime &vdtime)
{
	SYSTEMTIME time;
	FILETIME ftime = {0}, ltime = {0};
	ZeroMemory( &time, sizeof( time ) );
	time.wYear   = (WORD)(vdtime.Year + 1900);
	time.wMonth  = vdtime.Month;
	time.wDay    = vdtime.Day;
	time.wHour   = vdtime.Hour;
	time.wMinute = vdtime.Minute;
	time.wSecond = vdtime.Second;
	SystemTimeToFileTime( &time, &ftime );

	// Apply time-zone shift to convert to UTC
	int nTimeZoneShift = vdtime.Zone / 4;
	if (nTimeZoneShift != 0)
	{
		__int64* li = (__int64*)&ftime;
		*li -= nTimeZoneShift * (__int64)10000000*60*60;
	}

	//FileTimeToLocalFileTime(&ftime, &ltime);
	ltime = ftime;

	return ltime;
}

int ExtractFile(IsoImage *image, Directory *dir, const wchar_t *destPath, const ExtractProcessCallbacks* epc)
{
	int result = SER_SUCCESS;

	HANDLE hFile = CreateFileW(destPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		result = SER_ERROR_WRITE;
	}
	else
	{
		bool xbox = dir->VolumeDescriptor->XBOX;
    
		DWORD size = xbox ? dir->XBOXRecord.DataLength : (DWORD)dir->Record.DataLength;
		DWORD block = xbox ? dir->XBOXRecord.LocationOfExtent : (DWORD)dir->Record.LocationOfExtent;
		DWORD sector = xbox ? 0x800 : (WORD)dir->VolumeDescriptor->VolumeDescriptor.LogicalBlockSize;
		DWORD block_increment = 1;

		if( sector == image->RealBlockSize ) // if logical block size == real block size then read/write by 10 blocks
		{
			sector *= 10;
			block_increment = 10;
		}

		char* buffer = (char*) malloc( sector );
		if( !buffer )
		{
			CloseHandle(hFile);
			return SER_ERROR_SYSTEM;
		}

		DWORD initial_size = size;
		for( ; (int)size >= 0; size -= sector, block += block_increment )
		{
			DWORD cur_size = min( sector, size );
			if( cur_size && ReadBlock( image, block, cur_size, buffer ) != cur_size )
			{
				CloseHandle(hFile);
				free( buffer );
				DeleteFileW(destPath);

				return SER_ERROR_READ;
			}

			if( cur_size )
			{
				DWORD write;
				WriteFile( hFile, buffer, cur_size, &write, 0 );
				if( write != cur_size )
				{
					CloseHandle( hFile );
					free( buffer );
					DeleteFileW(destPath);

					return SER_ERROR_WRITE;
				}
			}

			if (initial_size && epc && epc->FileProgress)
			{
				ProgressContext* pctx = (ProgressContext*) epc->signalContext;
				pctx->nProcessedBytes += cur_size;
				
				int nProgressVal = ((__int64)(initial_size - size) * 100) / initial_size;
				if (nProgressVal != pctx->nCurrentFileProgress)
				{
					pctx->nCurrentFileProgress = nProgressVal;
					
					if (!epc->FileProgress(epc->signalContext))
					{
						CloseHandle(hFile);
						free(buffer);
						DeleteFileW(destPath);

						return SER_USERABORT;
					}
				}
			}
		} //for

		if(!xbox)
		{
			FILETIME ftime = VolumeDateTimeToFileTime(dir->Record.RecordingDateAndTime);
			SetFileTime( hFile, &ftime, NULL, &ftime );
		}

		free( buffer );
		CloseHandle(hFile);

		SetFileAttributesW(destPath, GetDirectoryAttributes(dir));
	}

	return result;
}
