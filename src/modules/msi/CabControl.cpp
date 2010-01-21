#include "StdAfx.h"
#include "CabControl.h"

#include "cab/mspack.h"

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

static bool DecodeCabAttributes(mscabd_file *cabfile, DWORD &fileAttr)
{
	fileAttr = 0;
	
	if (cabfile->attribs & MSCAB_ATTRIB_RDONLY)
		fileAttr |= FILE_ATTRIBUTE_READONLY;
	if (cabfile->attribs & MSCAB_ATTRIB_ARCH)
		fileAttr |= FILE_ATTRIBUTE_ARCHIVE;
	if (cabfile->attribs & MSCAB_ATTRIB_HIDDEN)
		fileAttr |= FILE_ATTRIBUTE_HIDDEN;
	if (cabfile->attribs & MSCAB_ATTRIB_SYSTEM)
		fileAttr |= FILE_ATTRIBUTE_SYSTEM;

	return true;
}

static FILETIME CabTimeToFileTime(mscabd_file *cabfile)
{
	FILETIME res = {0};

	SYSTEMTIME stime;
	memset(&stime, 0, sizeof(stime));
	stime.wYear = cabfile->date_y;
	stime.wMonth = cabfile->date_m;
	stime.wDay = cabfile->date_d;
	stime.wHour = cabfile->time_h;
	stime.wMinute = cabfile->time_m;
	stime.wSecond = cabfile->time_s;
	SystemTimeToFileTime(&stime, &res);

	return res;
}

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

	char* szAnsiSourceName = (char *) malloc(MAX_PATH);
	memset(szAnsiSourceName, 0, MAX_PATH);
	WideCharToMultiByte(CP_ACP, 0, sourceFileName, wcslen(sourceFileName), szAnsiSourceName, MAX_PATH, NULL, NULL);

	// Unpack library expects disk file names in UTF-8 (after custom patch)
	size_t nDestNameLen = wcslen(destFilePath) * 2 + 1;
	char* szAnsiDestName = (char *) malloc(nDestNameLen);
	memset(szAnsiDestName, 0, nDestNameLen);
	WideCharToMultiByte(CP_UTF8, 0, destFilePath, wcslen(destFilePath), szAnsiDestName, nDestNameLen, NULL, NULL);
	
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

	// Reopen destination file to set required attributes
	if (cabfile)
	{
		HANDLE hFile = CreateFile(destFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			FILETIME ft = CabTimeToFileTime(cabfile);
			SetFileTime(hFile, NULL, NULL, &ft);
			CloseHandle(hFile);

			DWORD attr;
			DecodeCabAttributes(cabfile, attr);
			if (attr != 0) SetFileAttributes(destFilePath, attr);
		}
	}

	return result;
}

bool CCabControl::GetFileAttributes(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, WIN32_FIND_DATAW &fd)
{
	if (!cabName || !cabPath || !sourceFileName)
		return false;

	CabCacheItem* item = getCacheItem(cabName, cabPath);
	if (!item)
		return false;

	char* szAnsiSourceName = (char *) malloc(MAX_PATH);
	memset(szAnsiSourceName, 0, MAX_PATH);
	WideCharToMultiByte(CP_ACP, 0, sourceFileName, wcslen(sourceFileName), szAnsiSourceName, MAX_PATH, NULL, NULL);

	bool fResult = false;
	
	mscabd_file *cabfile = item->data->files;
	while (cabfile)
	{
		if (strcmp(cabfile->filename, szAnsiSourceName) == 0)
		{
			memset(&fd, 0, sizeof(fd));
			wcscpy_s(fd.cFileName, MAX_PATH, sourceFileName);
			fd.nFileSizeLow = cabfile->length;
			DecodeCabAttributes(cabfile, fd.dwFileAttributes);
			fd.ftLastWriteTime = CabTimeToFileTime(cabfile);
			
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
	
	newItem->fileName = (char *) malloc(MAX_PATH);
	memset(newItem->fileName, 0, MAX_PATH);
	WideCharToMultiByte(CP_UTF8, 0, cabPath, wcslen(cabPath), newItem->fileName, MAX_PATH, NULL, NULL);

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