#include "StdAfx.h"
#include "..\ModuleDef.h"
#include "iso_tc.h"

#define NAME_PATH_BUFSIZE 2048

DWORD GetDirectoryAttributes(Directory* dir);

int ExtractFile(IsoImage *image, Directory *dir, const wchar_t *destPath, const ExtractProcessCallbacks* epc)
{
	wchar_t strDestFilePath[NAME_PATH_BUFSIZE] = {0};
	wcscpy_s(strDestFilePath, NAME_PATH_BUFSIZE, destPath);
	wcscat_s(strDestFilePath, NAME_PATH_BUFSIZE, dir->FileName);

	int result = SER_SUCCESS;

	HANDLE hFile = CreateFileW(strDestFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
				DeleteFileW(strDestFilePath);

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
					DeleteFileW(strDestFilePath);

					return SER_ERROR_WRITE;
				}
			}

			if (initial_size && epc && epc->Progress)
			{
				int progress = (initial_size - size) * 100 / initial_size;
				if (!epc->Progress(progress, epc->signalContext))
				{
					CloseHandle(hFile);
					free(buffer);
					DeleteFileW(strDestFilePath);

					return SER_USERABORT;
				}
			}
		}

		if(!xbox)
		{
			FILETIME   dtime;
			SYSTEMTIME time;
			FILETIME   ftime;
			ZeroMemory( &time, sizeof( time ) );
			time.wYear   = (WORD)(dir->Record.RecordingDateAndTime.Year + 1900);
			time.wMonth  = dir->Record.RecordingDateAndTime.Month;
			time.wDay    = dir->Record.RecordingDateAndTime.Day;
			time.wHour   = dir->Record.RecordingDateAndTime.Hour;
			time.wMinute = dir->Record.RecordingDateAndTime.Minute;
			time.wSecond = dir->Record.RecordingDateAndTime.Second;

			SystemTimeToFileTime( &time, &ftime );
			LocalFileTimeToFileTime( &ftime, &dtime );
			//dtime = ftime;
			SetFileTime( hFile, &dtime, &dtime, &dtime );
		}

		free( buffer );
		CloseHandle(hFile);

		SetFileAttributesW(strDestFilePath, GetDirectoryAttributes(dir));
	}

	return result;
}
