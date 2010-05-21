#ifndef common_h__
#define common_h__

typedef intptr_t X2FDLONG; // 32bits on W32, 64 bits on W64, signed
typedef uintptr_t X2FDULONG; // 32bits on W32, 64 bits on W64, unsigned

typedef unsigned char byte;

// used by X2FD_FindFirst/Next
struct X2FILEINFO
{
	char szFileName[260];
	X2FDLONG mtime;
	X2FDLONG size;
	X2FDLONG BinarySize;
	int flags;
};

void ParseCATPath(const char *pszName, char **ppszCATName, char **ppszFile);

bool FileExists(const wchar_t* path);
void UnixTimeToFileTime(time_t t, LPFILETIME pft);

const char* GetFileName(const char* path);
const wchar_t* GetFileName(const wchar_t* path);
wchar_t* GetFileExt(wchar_t* path);

#endif // common_h__
