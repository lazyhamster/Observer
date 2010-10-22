#include "StdAfx.h"
#include "CabControl.h"

extern "C"
{
#include "cab/mspack.h"
#include "cab/system.h"
};

struct CabCacheItem
{
	mscab_decompressor* decomp;
	mscabd_cabinet* data;
	char realFilePath[MAX_PATH];

	CabCacheItem()
	{
		decomp = NULL;
		data = NULL;
		memset(realFilePath, 0, MAX_PATH);
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

struct openfile_s
{
	MSIHANDLE hMSI;
	const wchar_t* streamName;
};

#define MEM_CACHE_CHUNK_SIZE (64*1024)

struct mspack_file_s
{
	wchar_t* streamName;
	
	MSIHANDLE hQuery;
	MSIHANDLE hStreamRec;
	int nPos;
	int nStreamSize;

	char* pMemCache;
	int nMemCacheSize;
};

static struct mspack_file *msp_fake_open(struct mspack_system *thisPtr,	char *filename, int mode)
{
	if (mode != MSPACK_SYS_OPEN_READ)
		return NULL;
	
	UINT res;
	MSIHANDLE hQueryStream, hStreamRec;
	openfile_s* openf = (openfile_s*) filename;

	res = MsiDatabaseOpenViewW(openf->hMSI, L"SELECT * FROM _Streams", &hQueryStream);
	if (res != ERROR_SUCCESS) return NULL;

	res = MsiViewExecute(hQueryStream, 0);
	if (res != ERROR_SUCCESS) return NULL;

	mspack_file* pResult = NULL;

	// Go through streams list and copy to temp folder
	DWORD nCellSize;
	wchar_t wszStreamName[256] = {0};
	while ((res = MsiViewFetch(hQueryStream, &hStreamRec)) != ERROR_NO_MORE_ITEMS)
	{
		if (res != ERROR_SUCCESS) break;

		nCellSize = sizeof(wszStreamName)/sizeof(wszStreamName[0]);
		res = MsiRecordGetStringW(hStreamRec, 1, wszStreamName, &nCellSize);
		if (res != ERROR_SUCCESS) break;

		if (wcscmp(wszStreamName, openf->streamName + 1) == 0)
		{
			mspack_file_s* fh = (mspack_file_s*) malloc(sizeof(struct mspack_file_s));
			fh->streamName = _wcsdup(openf->streamName);
			
			fh->hQuery = hQueryStream;
			fh->hStreamRec = hStreamRec;
			fh->nPos = 0;
			fh->nStreamSize = MsiRecordDataSize(hStreamRec, 2);

			fh->pMemCache = NULL;
			fh->nMemCacheSize = 0;

			pResult = (mspack_file *) fh;
			break;
		}

		MsiCloseHandle(hStreamRec);
	}

	if (!pResult)
	{
		MsiCloseHandle(hStreamRec);
		MsiCloseHandle(hQueryStream);
	}

	return pResult;
}

static void msp_fake_close(struct mspack_file *file)
{
	mspack_file_s* fh = (mspack_file_s*) file;
	if (fh)
	{
		MsiCloseHandle(fh->hStreamRec);
		MsiCloseHandle(fh->hQuery);
		if (fh->pMemCache) free(fh->pMemCache);
		free(fh->streamName);
		free(fh);
	}
}

static int msp_fake_read(struct mspack_file *file, void *buffer, int bytes)
{
	mspack_file_s* fh = (mspack_file_s*) file;
	if (fh)
	{
		// Cache data to buffer
		int nRequredSize = fh->nPos + bytes;
		if ((nRequredSize > fh->nMemCacheSize) && (fh->nMemCacheSize < fh->nStreamSize))
		{
			int nNewSize = ((nRequredSize / MEM_CACHE_CHUNK_SIZE) + 1) * MEM_CACHE_CHUNK_SIZE;
			if (nNewSize > fh->nStreamSize) nNewSize = fh->nStreamSize;
			
			if (nNewSize <= fh->nMemCacheSize) return -1;

			fh->pMemCache = (char*) realloc(fh->pMemCache, nNewSize);
			if (fh->pMemCache == NULL) return -1;

			DWORD dwCopySize = nNewSize - fh->nMemCacheSize;
			UINT res = MsiRecordReadStream(fh->hStreamRec, 2, fh->pMemCache + fh->nMemCacheSize, &dwCopySize);
			if ((res != ERROR_SUCCESS) || (dwCopySize != nNewSize - fh->nMemCacheSize)) return -1;

			fh->nMemCacheSize = nNewSize;
		}

		// Calculate available data
		int nCopySize = bytes;
		if ((fh->nPos + nCopySize) > fh->nStreamSize)
			nCopySize = fh->nStreamSize - fh->nPos;
		
		// Copy data from memory cache
		memcpy(buffer, fh->pMemCache + fh->nPos, nCopySize);
		fh->nPos += nCopySize;
		return nCopySize;
	}

	return -1;
}

static int msp_fake_write(struct mspack_file *file, void *buffer, int bytes)
{
	//Don't need this
	return -1;
}

static int msp_fake_seek(struct mspack_file *file, off_t offset, int mode)
{
	mspack_file_s* fh = (mspack_file_s*) file;
	if (fh)
	{
		switch (mode)
		{
			case MSPACK_SYS_SEEK_START:
				fh->nPos = offset;
				return 0;
			case MSPACK_SYS_SEEK_CUR:
				fh->nPos += offset;
				return 0;
			case MSPACK_SYS_SEEK_END:
				if (offset <= 0)
				{
					fh->nPos = fh->nStreamSize + offset;
					return 0;
				}
				break;
		}
	}
	
	return -1;
}

static off_t msp_fake_tell(struct mspack_file *file)
{
	mspack_file_s* fh = (mspack_file_s*) file;
	if (fh)
		return fh->nPos;

	return -1;
}

static void msp_fake_msg(struct mspack_file *file, char *format, ...) {
	va_list ap;
	if (file) fprintf(stderr, "%S: ", ((struct mspack_file_s *) file)->streamName);
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fputc((int) '\n', stderr);
	fflush(stderr);
}

static void *msp_alloc(struct mspack_system *thisPtr, size_t bytes)
{
	return malloc(bytes);
}

static void msp_free(void *buffer)
{
	free(buffer);
}

static void msp_copy(void *src, void *dest, size_t bytes)
{
	memcpy(dest, src, bytes);
}

static struct mspack_system msp_fake = {
	&msp_fake_open, &msp_fake_close, &msp_fake_read,  &msp_fake_write, &msp_fake_seek,
	&msp_fake_tell, &msp_fake_msg, &msp_alloc, &msp_free, &msp_copy, NULL
};

//////////////////////////////////////////////////////////////////////////

CCabControl::CCabControl(void)
{
	m_hOwner = 0;
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

	// Unpack library expects disk file names in UTF-8 (after custom patch)
	size_t nDestNameLen = wcslen(destFilePath) * 2 + 1;
	char* szAnsiDestName = (char *) malloc(nDestNameLen);
	memset(szAnsiDestName, 0, nDestNameLen);
	WideCharToMultiByte(CP_UTF8, 0, destFilePath, -1, szAnsiDestName, (int)nDestNameLen, NULL, NULL);
	
	int result = FALSE;

	mscabd_file *cabfile = FindFileByName(item->data->files, sourceFileName);
	if (cabfile)
	{
		int xerr = item->decomp->extract(item->decomp, cabfile, szAnsiDestName);
		result = (xerr == MSPACK_ERR_OK);
	}
		
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
		if (cabPath && *cabPath && !result->realFilePath[0])
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

	mspack_system* mspsys = (!cabPath || !*cabPath) ? &msp_fake : mspack_default_system;
	mscab_decompressor* newDecomp = mspack_create_cab_decompressor(mspsys);
	if (!newDecomp) return NULL;

	CabCacheItem *newItem = new CabCacheItem();
	newItem->decomp = newDecomp;
	
	char* openFileName = NULL;
	openfile_s ofile;

	if (cabPath != NULL)
	{
		memset(newItem->realFilePath, 0, MAX_PATH);
		WideCharToMultiByte(CP_UTF8, 0, cabPath, -1, newItem->realFilePath, MAX_PATH, NULL, NULL);
		openFileName = newItem->realFilePath;
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
