#include "StdAfx.h"
#include "CabControl.h"

#include <mspack.h>

struct CabCacheItem
{
	mscab_decompressor* decomp;
	mscabd_cabinet* data;
	std::wstring realPath;

	CabCacheItem()
	{
		decomp = NULL;
		data = NULL;
	}

	~CabCacheItem()
	{
		if (decomp)
		{
			if (data) decomp->close(decomp, data);
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

static mscabd_file* FindFileByName(mscabd_file* first, const wchar_t* name)
{
	char szAnsiName[MAX_PATH] = {0};
	WideCharToMultiByte(CP_ACP, 0, name, -1, szAnsiName, MAX_PATH, NULL, NULL);
	
	mscabd_file* current = first;
	while (current)
	{
		if (_stricmp(current->filename, szAnsiName) == 0)
			return current;
		
		current = current->next;
	}
	
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

extern struct mspack_system *mspack_direct_system;
extern struct mspack_system *mspack_fake_system;

//////////////////////////////////////////////////////////////////////////

CCabControl::CCabControl(void)
{
	m_hOwner = 0;
}

CCabControl::~CCabControl(void)
{
	for (auto iter = m_mCabCache.begin(); iter != m_mCabCache.end(); iter++)
	{
		CabCacheItem* item = iter->second;
		delete item;		
	}
	m_mCabCache.clear();
}

bool CCabControl::ExtractFile( const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, const wchar_t* destFilePath )
{
	if (!cabName || !cabPath || !sourceFileName)
		return false;
	
	CabCacheItem* item = getCacheItem(cabName, cabPath);
	if (!item) return false;

	bool result = false;

	mscabd_file *cabfile = FindFileByName(item->data->files, sourceFileName);
	if (cabfile)
	{
		const char* szAnsiDestName = (const char*) destFilePath;
		int xerr = item->decomp->extract(item->decomp, cabfile, szAnsiDestName);
		result = (xerr == MSPACK_ERR_OK);
	}
		
	return result;
}

bool CCabControl::GetFileAttributes(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, WIN32_FIND_DATAW &fd)
{
	if (!cabName || !*cabName || !sourceFileName)
		return false;

	CabCacheItem* item = getCacheItem(cabName, cabPath);
	if (!item)
		return false;

	bool fResult = false;
	
	mscabd_file *cabfile = FindFileByName(item->data->files, sourceFileName);
	if (cabfile)
	{
		memset(&fd, 0, sizeof(fd));
		wcscpy_s(fd.cFileName, MAX_PATH, sourceFileName);
		fd.nFileSizeLow = cabfile->length;
		DecodeCabAttributes(cabfile, fd.dwFileAttributes);
		fd.ftLastWriteTime = CabTimeToFileTime(cabfile);
			
		fResult = true;
	}

	return fResult;
}

CabCacheItem* CCabControl::getCacheItem( const wchar_t* cabName, const wchar_t* cabPath )
{
	if (!cabName || !*cabName)
		return NULL;
	
	CabCacheItem* result = m_mCabCache[cabName];
	if (result)
	{
		// We already have fake contents but now need real one
		if (cabPath && *cabPath && (result->realPath.length() == 0))
		{
			delete result;
			result = NULL;
			m_mCabCache[cabName] = NULL;
		}
		else
		{
			return result;
		}
	}

	mspack_system* mspsys = (!cabPath || !*cabPath) ? mspack_fake_system : mspack_direct_system;
	mscab_decompressor* newDecomp = mspack_create_cab_decompressor(mspsys);
	if (!newDecomp) return NULL;

	CabCacheItem *newItem = new CabCacheItem();
	newItem->decomp = newDecomp;
	
	char* openFileName = NULL;
	openfile_s ofile;

	if (cabPath != NULL)
	{
		newItem->realPath = cabPath;
		openFileName = (char*) newItem->realPath.c_str();
	}
	else
	{
		ofile.hMSI = m_hOwner;
		ofile.streamName = cabName;
		openFileName = (char *) &ofile;
	}

	mscabd_cabinet* cabData = newDecomp->open(newDecomp, openFileName);
	if (cabData)
	{
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
