#include "stdafx.h"

bool ReadBuffer(HANDLE file, LPVOID buffer, DWORD bufferSize)
{
	DWORD dwBytes;
	return ReadFile(file, buffer, bufferSize, &dwBytes, NULL) && (dwBytes == bufferSize);
}

bool SeekFile(HANDLE file, DWORD position)
{
	DWORD filePtr = SetFilePointer(file, position, NULL, FILE_BEGIN);
	return (filePtr != INVALID_SET_FILE_POINTER) && (filePtr == position);
}

void CombinePath(char* buffer, size_t bufferSize, int numParts, ...)
{
	va_list argptr;
	va_start(argptr, numParts);
	for (int i = 0; i < numParts; i++)
	{
		char* next = va_arg(argptr, char*);
		if (next && *next)
		{
			if (buffer[0])
				strcat_s(buffer, bufferSize, "\\");

			strcat_s(buffer, bufferSize, next);
		}
	}
	va_end(argptr);
}
