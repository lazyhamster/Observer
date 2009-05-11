// StreamUtils.cpp

#include "StdAfx.h"
#include "Types.h"
#include "StreamUtils.h"

static const UInt32 kBlockSize = ((UInt32)1 << 31);

bool ReadStream(HANDLE hFile, void *data, size_t *processedSize)
{
	size_t size = *processedSize;
	*processedSize = 0;
	while (size != 0)
	{
		UInt32 curSize = (size < kBlockSize) ? (UInt32)size : kBlockSize;
		DWORD processedSizeLoc;
		BOOL res = ReadFile(hFile, data, curSize, &processedSizeLoc, NULL);
		*processedSize += processedSizeLoc;
		data = (void *)((Byte *)data + processedSizeLoc);
		size -= processedSizeLoc;
		if (!res)
			return false;
		if (processedSizeLoc == 0)
			return true;
	}
	return true;
}

bool ReadStream_FALSE(HANDLE hFile, void *data, size_t size)
{
	size_t processedSize = size;
	if (ReadStream(hFile, data, &processedSize))
		return (size == processedSize);
	else
		return false;
}

bool SeekStream(HANDLE hFile, UInt64 moveDistance, int moveOrigin, UInt64 *currentPosition)
{
	LONG distLow = (moveDistance & 0xFFFFFFFF);
	LONG distHigh = (moveDistance >> 32);

	distLow = SetFilePointer(hFile, distLow, &distHigh, moveOrigin);

	if ((distLow == INVALID_SET_FILE_POINTER) && GetLastError())
		return false;

	if (currentPosition)
		*currentPosition = distLow + ((UInt64) distHigh << 32);

	return true;
}

UInt64 StreamSize(HANDLE hFile)
{
	DWORD fsizeHigh;
	DWORD fsize = GetFileSize(hFile, &fsizeHigh);

	return fsize + ((UInt64)fsizeHigh << 32);
}