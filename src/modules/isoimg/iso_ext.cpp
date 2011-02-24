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

DWORD GetDirectoryAttributes(Directory* dir)
{
	DWORD dwAttr = 0;

	if (dir->VolumeDescriptor->XBOX)
	{
		if (dir->XBOXRecord.FileFlags & XBOX_DIRECTORY)
			dwAttr |= FILE_ATTRIBUTE_DIRECTORY;
		if (dir->XBOXRecord.FileFlags & XBOX_HIDDEN)
			dwAttr |= FILE_ATTRIBUTE_HIDDEN;
		if (dir->XBOXRecord.FileFlags & XBOX_ARCHIVE)
			dwAttr |= FILE_ATTRIBUTE_ARCHIVE;
		if (dir->XBOXRecord.FileFlags & XBOX_SYSTEM)
			dwAttr |= FILE_ATTRIBUTE_SYSTEM;

		if (dwAttr == 0)
			dwAttr = FILE_ATTRIBUTE_NORMAL;
	}
	else
	{
		dwAttr = FILE_ATTRIBUTE_ARCHIVE;
		if (dir->Record.FileFlags & FATTR_DIRECTORY)
			dwAttr |= FILE_ATTRIBUTE_DIRECTORY;
		if (dir->Record.FileFlags & FATTR_HIDDEN)
			dwAttr |= FILE_ATTRIBUTE_HIDDEN;
	}

	return dwAttr;
}

bool IsDirectory(Directory *dir)
{
	if (dir->VolumeDescriptor->XBOX)
	{
		return (dir->XBOXRecord.FileFlags & XBOX_DIRECTORY) > 0;
	}
	else
	{
		return (dir->Record.FileFlags & FATTR_DIRECTORY) > 0;
	}
}

DWORD GetSize(Directory *dir)
{
	if (dir->VolumeDescriptor->XBOX)
		return (DWORD) dir->XBOXRecord.DataLength;
	else
		return (DWORD) dir->Record.DataLength;
}

FILETIME StringToTime(const char* data)
{
	FILETIME res = {0};

	SYSTEMTIME st = {0};
	int valnum = sscanf(data, "%4d%2d%2d%2d%2d%2d", &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
	if (valnum >= 3) SystemTimeToFileTime(&st, &res);

	return res;
}

int ExtractFile(IsoImage *image, Directory *dir, const wchar_t *destPath, const ExtractProcessCallbacks* epc)
{
	int result = SER_SUCCESS;

	HANDLE hFile = CreateFileW(destPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	bool xbox = dir->VolumeDescriptor->XBOX;

	__int64 size = xbox ? dir->XBOXRecord.DataLength : (DWORD)dir->Record.DataLength;
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

	for( ; size >= 0; size -= sector, block += block_increment )
	{
		DWORD cur_size = (DWORD) min( sector, size );
		if( cur_size && ReadBlock( image, block, cur_size, buffer ) != cur_size )
		{
			result = SER_ERROR_READ;
			break;
		}

		if( cur_size )
		{
			DWORD write;
			if( !WriteFile( hFile, buffer, cur_size, &write, 0 ) || (write != cur_size) )
			{
				result = SER_ERROR_WRITE;
				break;
			}

			if (epc && epc->FileProgress)
			{
				if (!epc->FileProgress(epc->signalContext, cur_size))
				{
					result = SER_USERABORT;
					break;
				}
			}
		}
	} //for

	if(!xbox && (result == SER_SUCCESS))
	{
		FILETIME ftime = VolumeDateTimeToFileTime(dir->Record.RecordingDateAndTime);
		SetFileTime( hFile, &ftime, NULL, &ftime );
	}

	free( buffer );
	CloseHandle(hFile);

	if (result == SER_SUCCESS)
		SetFileAttributesW(destPath, GetDirectoryAttributes(dir));
	else
		DeleteFileW(destPath);

	return result;
}
