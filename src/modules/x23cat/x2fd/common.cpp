#include "StdAfx.h"
//---------------------------------------------------------------------------------
void ParseCATPath(const char *pszName, char **ppszCATName, char **ppszFile)
{
	const char *pos=strstr(pszName, "::"), *end=(char*)(pszName + strlen(pszName));
	if(pos==0){
		pos=((char*)pszName) - 2;
		*ppszFile=new char[end - pos - 1];
		*ppszCATName=0;
	}
	else{
		*ppszCATName=new char[pos-pszName + 1];
		memcpy(*ppszCATName, pszName, pos-pszName);
		(*ppszCATName)[pos-pszName]=0;
		*ppszFile=new char[end-pos-2 + 1];
	}
	strcpy(*ppszFile, pos + 2);
}
//---------------------------------------------------------------------------------
