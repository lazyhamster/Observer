#include "StdAfx.h"
#include <stdarg.h>
#include <math.h>
#include "strutils.h"
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
