#include "StdAfx.h"
#include <stdarg.h>
#include <errno.h>
#include "common/strutils.h"
#include "common/gzip.h"

#include "x2fd.h"

char *errors[] = {"No error.", // 0
									"Invalid handle.", // 1
									"Cannot seek to specified position.", // 2
									"Bad seek mode.", // 3
									
									"Unknown error while opening file.", // 4
									"Cannot open file - access denied or file locked.", // 5
									"File already exist.", // 6
									"File does not exist.", // 7
									
									"Bad file mode.", // 8
									"Bad data in file.", // 9
									"File is empty.", // 10
									"File is locked", // 11
									
									"CAT archive is not open.", // 12
									"No entry in CAT file.", // 13
									"Cannot find DAT file.", // 14
									"Invalid size in CAT entry.", // 15
									"Files in CAT are locked.",
									"File name syntax is incorrect.",
									
									"Internal error.",
									"Cannot allocate memory.",
									"Bad flags specified.",
									
									"GZIP - reserved flag bits are set.",
									"GZIP - no GZIP header found.",
									"GZIP - input stream too small.",
									"GZIP - data CRC does not match.",
									"GZIP - header CRC does not match",
									"GZIP - unsupported compression.",
									"GZIP - deflate/inflate failed.",
									};
#define ERR_COUNT 28

static char g_szLastError[255]="";
static int g_nErrCode=0;
//---------------------------------------------------------------------------------
void error(int code)
{
	g_nErrCode=code;
}
//---------------------------------------------------------------------------------
void error(const char *pszError, ...)
{
	va_list ap;
	va_start(ap, pszError);
	_vsnprintf(g_szLastError, sizeof(g_szLastError), pszError, ap);
	va_end(ap);
	error(X2FD_E_MSG);
}
//---------------------------------------------------------------------------------
int error()
{
	return g_nErrCode;
}
//---------------------------------------------------------------------------------
int fileerror(int ferr) 
{  
	int e;
	switch(ferr){
		case EACCES:
			e=X2FD_E_FILE_ACCESS;
			break;
		case EEXIST:
			e=X2FD_E_FILE_EXIST;
			break;
		case ENOENT:
			e=X2FD_E_FILE_NOENTRY;
			break;
		default:
			e=X2FD_E_FILE_ERROR;
			break;
	}
	return e;
}
//---------------------------------------------------------------------------------
const char * errormsg(int code)
{
	if(code==X2FD_E_MSG)
		return g_szLastError;
	else if(code >=0 && code < ERR_COUNT)
		return errors[code];
	else
		return 0;
}
//---------------------------------------------------------------------------------
void clrerr()
{
	g_szLastError[0]=0;
	g_nErrCode=0;
}
//---------------------------------------------------------------------------------
int TranslateGZError(int GZError)
{
	int err;
	switch(GZError){
		case GZ_E_OK:
			err=0;
			break;
		case GZ_E_FLAGS:
			err=X2FD_E_GZ_FLAGS;
			break;
		case GZ_E_HEADER:
			err=X2FD_E_GZ_HEADER;
			break;
		case GZ_E_TOOSMALL:
			err=X2FD_E_GZ_TOOSMALL;
			break;
		case GZ_E_CRC:
			err=X2FD_E_GZ_CRC;
			break;
		case GZ_E_HCRC:
			err=X2FD_E_GZ_HCRC;
			break;
		case GZ_E_COMPRESSION:
			err=X2FD_E_GZ_COMPRESSION;
			break;
		case GZ_E_DEFLATE:
			err=X2FD_E_GZ_DEFLATE;
			break;
		case GZ_E_INTERNAL_ERROR:
			err=X2FD_E_ERROR;
			break;
	}
	return err;
}
//---------------------------------------------------------------------------------
