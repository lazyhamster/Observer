#ifndef Utils_h__
#define Utils_h__

bool ReadBuffer(HANDLE file, LPVOID buffer, DWORD bufferSize);

bool SeekFile(HANDLE file, DWORD position);

void CombinePath(char* buffer, size_t bufferSize, int numParts, ...);
void CombinePath(wchar_t* buffer, size_t bufferSize, int numParts, ...);

DWORD GetMajorVersion(DWORD fileVersion);

#endif // Utils_h__
