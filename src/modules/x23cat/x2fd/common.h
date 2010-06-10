#ifndef common_h__
#define common_h__

typedef intptr_t X2FDLONG; // 32bits on W32, 64 bits on W64, signed
typedef uintptr_t X2FDULONG; // 32bits on W32, 64 bits on W64, unsigned

typedef unsigned char byte;

// used by X2FD_FindFirst/Next
struct X2FILEINFO
{
	wchar_t szFileName[260];
	time_t mtime;
	X2FDLONG size;
	X2FDLONG BinarySize;
	int flags;
};

bool FileExists(const wchar_t* path);
void UnixTimeToFileTime(time_t t, LPFILETIME pft);

const wchar_t* GetFileName(const wchar_t* path);
wchar_t* GetFileExt(wchar_t* path);
const wchar_t* GetFileExt(const wchar_t* path);

#endif // common_h__
