#include "stdafx.h"
#include "CabSystem.h"

#include <MsiQuery.h>
#include <mspack.h>

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

static struct mspack_file *msp_fake_open(struct mspack_system *thisPtr,	const char *filename, int mode)
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

static void msp_fake_msg(struct mspack_file *file, const char *format, ...) {
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
	if (buffer)
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

struct mspack_system *mspack_fake_system = &msp_fake;
