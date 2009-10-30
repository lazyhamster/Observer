#include "StdAfx.h"
#include "CabControl.h"

#include "cab/mspack.h"

#define ANSI_STR_BUF_SIZE MAX_PATH

struct CabCacheItem
{
	mscab_decompressor* decomp;
	mscabd_cabinet* data;
	char* fileName;

	CabCacheItem()
	{
		decomp = NULL;
		data = NULL;
		fileName = NULL;
	}

	~CabCacheItem()
	{
		if (fileName)
			free(fileName);
		if (decomp)
		{
			if (data)
				decomp->close(decomp, data);
			mspack_destroy_cab_decompressor(decomp);
		}
	}
};

CCabControl::CCabControl(void)
{
}

CCabControl::~CCabControl(void)
{
	map<wstring, CabCacheItem*>::iterator iter;
	for (iter = m_mCabCache.begin(); iter != m_mCabCache.end(); iter++)
	{
		CabCacheItem* item = iter->second;
		delete item;		
	}
	m_mCabCache.clear();
}

int CCabControl::ExtractFile( const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, const wchar_t* destFilePath )
{
	if (!cabName || !cabPath || !sourceFileName)
		return FALSE;
	
	CabCacheItem* item = getCacheItem(cabName, cabPath);
	if (!item)
		return FALSE;

	char* szAnsiSourceName = (char *) malloc(ANSI_STR_BUF_SIZE);
	memset(szAnsiSourceName, 0, ANSI_STR_BUF_SIZE);
	WideCharToMultiByte(CP_ACP, 0, sourceFileName, wcslen(sourceFileName), szAnsiSourceName, ANSI_STR_BUF_SIZE, NULL, NULL);

	char* szAnsiDestName = (char *) malloc(ANSI_STR_BUF_SIZE);
	memset(szAnsiDestName, 0, ANSI_STR_BUF_SIZE);
	WideCharToMultiByte(CP_ACP, 0, destFilePath, wcslen(destFilePath), szAnsiDestName, ANSI_STR_BUF_SIZE, NULL, NULL);
	
	int result = FALSE;

	mscabd_file *cabfile = item->data->files;
	while (cabfile)
	{
		if (strcmp(cabfile->filename, szAnsiSourceName) == 0)
		{
			int xerr = item->decomp->extract(item->decomp, cabfile, szAnsiDestName);
			result = (xerr == MSPACK_ERR_OK);
			break;
		}
		
		cabfile = cabfile->next;
	}
		
	free(szAnsiSourceName);
	free(szAnsiDestName);

	return result;
}

bool CCabControl::GetFileAttributes(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, WIN32_FIND_DATAW &fd)
{
	if (!cabName || !cabPath || !sourceFileName)
		return false;

	CabCacheItem* item = getCacheItem(cabName, cabPath);
	if (!item)
		return false;

	char* szAnsiSourceName = (char *) malloc(ANSI_STR_BUF_SIZE);
	memset(szAnsiSourceName, 0, ANSI_STR_BUF_SIZE);
	WideCharToMultiByte(CP_ACP, 0, sourceFileName, wcslen(sourceFileName), szAnsiSourceName, ANSI_STR_BUF_SIZE, NULL, NULL);

	bool fResult = false;
	
	mscabd_file *cabfile = item->data->files;
	while (cabfile)
	{
		if (strcmp(cabfile->filename, szAnsiSourceName) == 0)
		{
			memset(&fd, 0, sizeof(fd));
			wcscpy_s(fd.cFileName, MAX_PATH, sourceFileName);
			fd.nFileSizeLow = cabfile->length;
			fd.dwFileAttributes = cabfile->attribs;

			SYSTEMTIME stime;
			memset(&stime, 0, sizeof(stime));
			stime.wYear = cabfile->date_y;
			stime.wMonth = cabfile->date_m;
			stime.wDay = cabfile->date_d;
			stime.wHour = cabfile->time_h;
			stime.wMinute = cabfile->time_m;
			stime.wSecond = cabfile->time_s;
			SystemTimeToFileTime(&stime, &fd.ftLastWriteTime);

			fResult = true;
			break;
		}

		cabfile = cabfile->next;
	}

	free(szAnsiSourceName);

	return fResult;
}

CabCacheItem* CCabControl::getCacheItem( const wchar_t* cabName, const wchar_t* cabPath )
{
	if (!cabName || !*cabName)
		return NULL;
	
	CabCacheItem* result = m_mCabCache[cabName];
	if (result)
		return result;
	
	mscab_decompressor* newDecomp = mspack_create_cab_decompressor(NULL);
	if (!newDecomp)
		return NULL;

	CabCacheItem *newItem = new CabCacheItem();
	
	newItem->fileName = (char *) malloc(ANSI_STR_BUF_SIZE);
	memset(newItem->fileName, 0, ANSI_STR_BUF_SIZE);
	WideCharToMultiByte(CP_ACP, 0, cabPath, wcslen(cabPath), newItem->fileName, ANSI_STR_BUF_SIZE, NULL, NULL);

	mscabd_cabinet* cabData = newDecomp->open(newDecomp, newItem->fileName);
	if (cabData)
	{
		newItem->decomp = newDecomp;
		newItem->data = cabData;
		
		m_mCabCache[cabName] = newItem;
		result = newItem;
	}
	else
	{
		delete newItem;
	}
	
	return result;
}