/*********************************************************
 *
 * STRUTILS char handling functions
 * modified: 2.9.2004 09:34
 *
 * define STRUTILS_WIN_WCHAR to use str2wchar and wchar2str
 *
 ********************************************************/

#ifndef STRUTILS_INCLUDED
#define STRUTILS_INCLUDED

#define STRUTILS_MACRO

//---------------------------------------------------------------------------------
size_t memrep(void *buffer, int c, int n, size_t count, size_t limit=0);
size_t strrep(char *string, char c, char n, size_t limit=0);
bool isinteger(const char *string);
char * strcat2(int num, ...);
char * ChangeFileExtension(const char *pszFileName, const char *pszExt);
const char * GetFileExtension(const char *pszFileName);

#ifdef STRUTILS_WIN_WCHAR
wchar_t * str2wchar(const char *sz);
char * wchar2str(const wchar_t *ws);
#endif
//---------------------------------------------------------------------------------
#ifdef STRUTILS_MACRO

#define strcreate(buffer, src) { delete[] buffer; \
buffer=new char[(src==NULL ? 0 : strlen(src)) + 1]; \
buffer[0]=0; \
if(src) strcpy(buffer, src); }

#else

inline void strcreate(char *&buffer, const char *src)
{
	buffer=new char[(src==NULL ? 0 : strlen(src)) + 1];
	buffer[0]=0;
	if(src) strcpy(buffer, src);
}
#endif
//---------------------------------------------------------------------------------
#ifdef STRUTILS_MACRO

#define wstrcreate(buffer, src) delete[] buffer; \
buffer=new wchar_t[(src==NULL ? 0 : wcslen(src)) + 1]; \
buffer[0]=0; \
if(src) wcscpy(buffer, src);

#else

inline void wstrcreate(wchar_t *&buffer, const wchar_t *src)
{
	buffer=new wchar_t[(src==NULL ? 0 : wcslen(src)) + 1];
	buffer[0]=0;
	if(src) wcscpy(buffer, src);
}
#endif
//---------------------------------------------------------------------------------
inline
char * trim(char *text)
{
	while(*text==' ' || *text==9)
		text++;
	
	int e=(int)strlen(text);
	for( ; e > 0 && text[e-1]==' ' || text[e-1]==9; e--)
		;
	
	text[e]=0;
	return text;
}
//---------------------------------------------------------------------------------
inline size_t memrep(void *buffer, int c, int n, size_t count, size_t limit)
{
	char *pos=(char*)buffer, *end=(char*)buffer + count;
	size_t rep=0;
	do{
		pos=(char*)memchr(pos, c, end - pos);
		if(pos) {
			*pos = (char)n;
			pos++;
			rep++;
			if(limit && rep >= limit) break;
		}
	}
	while(pos && (pos < end));
	return rep;
}
//---------------------------------------------------------------------------------
inline size_t strrep(char *string, char c, char n, size_t limit)
{
	return memrep(string, c, n, strlen(string), limit);
}
//---------------------------------------------------------------------------------
inline bool isinteger(const char *string)
{
	size_t i=0;
	size_t s=strlen(string);

	bool bRes = s > 0;
	if(bRes){
		if(string[0]=='+' || string[0]=='-') i++;
		for( ; i < s; ++i){
			if(string[i] < '0' || string[i] > '9'){
				bRes=false;
				break;
			}
		}
	}
	return bRes;
}
//---------------------------------------------------------------------------------
#ifdef STRUTILS_WIN_WCHAR
inline wchar_t * str2wchar(const char *sz)
{
	size_t len=strlen(sz);
	wchar_t *ws=new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, sz, (int)len, ws, (int)len);
	ws[len]=0;
	return ws;
}
//---------------------------------------------------------------------------------
inline char * wchar2str(const wchar_t *ws)
{
	size_t len=wcslen(ws);
	char *sz=new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, ws, (int)len, sz, (int)len, 0, 0);
	sz[len]=0;
	return sz;
}
//---------------------------------------------------------------------------------
#endif // defined(STRUTILS_WIN_WCHAR)

#endif // !defined(STRUTILS_INCLUDED)