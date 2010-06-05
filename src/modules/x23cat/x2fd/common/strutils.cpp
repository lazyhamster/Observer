#include "StdAfx.h"
#include "strutils.h"
//---------------------------------------------------------------------------------
wchar_t * ChangeFileExtension(const wchar_t *pszFileName, const wchar_t *pszExt)
{
	const wchar_t *ch, *end=pszFileName + wcslen(pszFileName);
	wchar_t *newfile;

	for(ch=end; ch >= pszFileName && *ch!='.' && *ch!='\\'; ch--)
	 ;;

	size_t len;
	if(*ch=='\\')
		len=end - pszFileName;
	else if(ch < pszFileName)
		len=end - ch - 1;
	else
		len=ch - pszFileName;
	size_t extlen=wcslen(pszExt);
	newfile=new wchar_t[len + extlen + 2];
	newfile[len + extlen + 1]=0;
	memcpy(newfile, pszFileName, len * sizeof(wchar_t));
	newfile[len]='.';
	memcpy(newfile + len + 1, pszExt, extlen * sizeof(wchar_t));
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
