#include "StdAfx.h"
#include <stdarg.h>
#include <math.h>
#include "strutils.h"
//---------------------------------------------------------------------------------
char ** strexplode(char separator, char *string, size_t *size, size_t limit)
{
	size_t len=strlen(string);
	char *pos=string, *old=string, *end = string + len;
	char **array;
	*size=memrep(string, separator, 0, len, limit);
	if(limit==0) (*size)++;
	array=new char*[*size];
	int i=0;
	size_t done=0;
	do{
		pos=(char*)memchr(pos, 0, end - pos);
		array[i++]=old;
		if(pos) old=++pos;
		if(limit && (++done >=limit)) break;
	}
	while(pos && pos <= end);

	return array;
}
//---------------------------------------------------------------------------------
char * ExtractFileName(const char *pszPath, bool bNoExtension)
{
	const char *end=pszPath + strlen(pszPath);
	const char *pos=strrchr(pszPath, '\\');
	if(pos==NULL) pos=(char*)pszPath - 1;
	char *str=new char[end - pos + 1];
	memcpy(str, ++pos, end - pos + 1);
	if(bNoExtension){
		char* pos2=strrchr(str, '.');
		if(pos2) *pos2=0;
	}
	return str;
}
//---------------------------------------------------------------------------------
#ifdef STRUTILS_WIN_WCHAR
wchar_t * ExtractFileNameW(const wchar_t *pszPath, bool bNoExtension)
{
	const wchar_t *end=pszPath + wcslen(pszPath);
	wchar_t *pos=wcsrchr(pszPath, '\\');
	if(pos==NULL) pos=(wchar_t*)pszPath - 1;
	wchar_t *str=new wchar_t[end - pos + 1];
	memcpy(str, ++pos, (end - pos + 1) * sizeof(wchar_t));
	if(bNoExtension){
		pos=wcsrchr(str, '.');
		if(pos) *pos=0;
	}
	return str;
}
//---------------------------------------------------------------------------------
#endif

char * ExtractFilePath(const char *pszPath)
{
	const char *pos=strrchr(pszPath, '\\');
	char *path=NULL;
	if(pos){
		path=new char[pos - pszPath + 1];
		memcpy(path, pszPath, pos - pszPath);
		path[pos - pszPath]=0;
	}
	return path;
}
//---------------------------------------------------------------------------------
char * strcat2(int num, ...)
{
	va_list ap=va_start(ap, num);
	const char *s;
	char *str;
	size_t l=0;
	for(int i=0; i < num; i++){
		s=va_arg(ap, char*);
		l+=strlen(s);
	}
	va_end(ap);
	str=new char[l + 1];
	str[0]=0;
	ap=va_start(ap, num);
	for(int i=0; i < num; i++){
		s=va_arg(ap, const char*);
		strcat(str, s);
	}
	va_end(ap);
	return str;
}
//---------------------------------------------------------------------------------
char ** lineexplode(char *pszBuffer, size_t size, size_t *count)
{
	char **ar;
	size_t cnt=0;
	// change LF to 0
	memrep(pszBuffer, 10, 0, size);
	// chanfe CR to 0
	memrep(pszBuffer, 13, 0, size);

	char *pos=pszBuffer, *old=pszBuffer, *end=pszBuffer + size;
	do{
		pos=(char*)memchr(old, 0, end - pos);
		cnt++;
		if(pos)
			while(*(++pos)==0) ;
		old=pos;
	}
	while(pos && ((end - pos) > 0));

	if(cnt==0) {
		*count=0;
		return NULL;
	}
	ar=new char*[cnt];
	size_t i=0;
	old=pos=pszBuffer;
	do{
		pos=(char*)memchr(old, 0, end - pos);
		ar[i++]=old;
		if(pos)
			while(*(++pos)==0) ;
		old=pos;
	}
	while(pos && ((end - pos) > 0));
	*count=cnt;
	return ar;
}
//---------------------------------------------------------------------------------
char * ChangeFileExtension(const char *pszFileName, const char *pszExt)
{
	const char *ch, *end=pszFileName + strlen(pszFileName);
	char *newfile;

	for(ch=end; ch >= pszFileName && *ch!='.' && *ch!='\\'; ch--)
	 ;;

	size_t len;
	if(*ch=='\\')
		len=end - pszFileName;
	else if(ch < pszFileName)
		len=end - ch - 1;
	else
		len=ch - pszFileName;
	size_t extlen=strlen(pszExt);
	newfile=new char[len + extlen + 2];
	newfile[len + extlen + 1]=0;
	memcpy(newfile, pszFileName, len);
	newfile[len]='.';
	memcpy(newfile + len + 1, pszExt, extlen);
	return newfile;
}
//---------------------------------------------------------------------------------
const char * GetFileExtension(const char *pszFileName)
{
	const char *ch, *end=pszFileName + strlen(pszFileName);

	for(ch=end; ch >= pszFileName && *ch!='.' && *ch!='\\'; ch--)
	 ;;

	if(ch < pszFileName || *ch=='\\')
		return end;
	else
		return ch + 1;
}
//---------------------------------------------------------------------------------
int hextoi(const char *str)
{
	size_t len, last;
	bool neg=false;
	while(*str!=0 && *str==' ')
		str++;
	
	// skip leading 0x....
	if(*str=='+')
		str++;
	else if(*str=='-') {
		str++;
		neg=true;
	}
	if(*str=='0' && ((str[1] | 0x20)=='x'))
		str+=2;
	
	int val=0, v;

	len=strlen(str);
	for(last=0; last < len; last++){
		int v=str[last] | 0x20;
		if(!((v >= '0' && v <='9') || (v >= 'a' && v <='f')))
			break;
	}
	
	for(size_t i=last - 1; (int)i >= 0; i--){
		v=str[i] | 0x20;
		if(v >= 'a')
			v-=87;
		else
			v-=48;
			
		val+=v * (int)pow((double) 16, (int)(last - i - 1));
	}
	if(neg)	val*=-1;
	return val;
}
//---------------------------------------------------------------------------------
bool ishexa(const char *str)
{
	size_t len, last;
	while(*str!=0 && *str==' ')
		str++;
	
	if(*str=='+' || *str=='-')
		str++;
	
	// skip leading 0x....
	if(*str=='0' && ((str[1] | 0x20)=='x'))
		str+=2;
	
	len=strlen(str);
	if(len==0) return false;
	for(last=0; last < len; last++){
		int v=str[last] | 0x20;
		if(!((v >= '0' && v <='9') || (v >= 'a' && v <='f')))
			return false;
	}
	
	return true;
}
//---------------------------------------------------------------------------------
