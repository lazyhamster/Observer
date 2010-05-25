#include "StdAfx.h"
#include "strutils.h"
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
