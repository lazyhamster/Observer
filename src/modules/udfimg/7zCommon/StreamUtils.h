// StreamUtils.h

#ifndef __STREAMUTILS_H
#define __STREAMUTILS_H

bool ReadStream(HANDLE hFile, void *data, size_t *size);
bool ReadStream_FALSE(HANDLE hFile, void *data, size_t size);
bool SeekStream(HANDLE hFile, UInt64 moveDistance, int moveOrigin, UInt64 *currentPosition);
UInt64 StreamSize(HANDLE hFile);

#endif
