#ifndef common_h__
#define common_h__

typedef unsigned char byte;
void ParseCATPath(const char *pszName, char **ppszCATName, char **ppszFile);

bool FileExists(const wchar_t* path);
void UnixTimeToFileTime(time_t t, LPFILETIME pft);
const char* GetFileName(const char* path);

#endif // common_h__
