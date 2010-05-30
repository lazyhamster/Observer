#include "StdAfx.h"
#include "file_io.h"

#include <time.h>

void io64::file::ctime2wintime(const time_t& time, FILETIME *wintime)
{
	// difference between midnight January 1, 1970 (time_t) and January 1, 1601 in 100 nanosecond intervals
	static const __int64 DELTA=116444736000000000i64;

	ULARGE_INTEGER i;
	i.QuadPart=time * 10000000i64 + DELTA; // convert to nanoseconds and add the difference
	
	wintime->dwHighDateTime=i.HighPart;
	wintime->dwLowDateTime=i.LowPart;
}

void io64::file::wintime2ctime(const FILETIME *wintime, time_t *time)
{
	// difference between midnight January 1, 1970 (time_t) and January 1, 1601 in 100 nanosecond intervals
	static const __int64 DELTA=116444736000000000i64;
	
	ULARGE_INTEGER i;
	i.HighPart=wintime->dwHighDateTime;
	i.LowPart=wintime->dwLowDateTime;
	
	*time=(time_t)((i.QuadPart - DELTA) / 10000000i64); // convert to seconds
}

/* 
ERROR_ALREADY_EXISTS - EEXIST
ERROR_ACCESS_DENIED - EACCESS
ERROR_FILE_NOT_FOUND - ENOENT
*/
void io64::file::werror()
{
	DWORD err=GetLastError();
	int c;
	switch(err){
		case ERROR_SUCCESS:
			c=0;
			break;
		case ERROR_INVALID_HANDLE:
			c=EBADF;
			break;
		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS: // both the same error
			c=EEXIST;
			break;
		case ERROR_ACCESS_DENIED:
			c=EACCES;
			break;
		case ERROR_FILE_NOT_FOUND:
			c=ENOENT;
			break;
		case ERROR_PATH_NOT_FOUND: // how to handle that?
		default:
			c=EUNKNOWN;
	};
	error(c);
}

bool io64::file::mtime(const time_t& time)
{
	error(0);
	FILETIME LastModificationTime;
	ctime2wintime(time, &LastModificationTime);
	
	if (!SetFileTime(handle(), 0, 0, &LastModificationTime)) {
		error(EINVAL);
		return false;
	}
	return true;
}

bool io64::file::atime(const time_t& time)
{
	error(0);
	FILETIME LastAccessTime;
	ctime2wintime(time, &LastAccessTime);
	if (!SetFileTime(handle(), 0, &LastAccessTime, 0)) {
		error(EINVAL);
		return false;
	}
	return true;
}

bool io64::file::ctime(const time_t& time)
{
	error(0);
	FILETIME CreationTime;
	ctime2wintime(time, &CreationTime);

	if (!SetFileTime(handle(), &CreationTime, 0, 0)) {
		error(EINVAL);
		return false;
	}
	return true;
}

bool io64::file::setTime(const time_t *ctime, const time_t *atime, const time_t *mtime)
{
	error(0);
	FILETIME CreationTime, WriteTime, AccessTime;
	if(ctime) ctime2wintime(*ctime, &CreationTime);
	if(atime) ctime2wintime(*atime, &AccessTime);
	if(mtime) ctime2wintime(*mtime, &WriteTime);

	if (!SetFileTime(handle(), 
		ctime ? &CreationTime : 0,
		atime ? &AccessTime : 0,
		mtime ? &WriteTime : 0) )
	{
		error(EINVAL);
		return false;
	}
	return true;
}

io64::file io64::file::open(const char *name, int mode, int createAttr, int share)
{
	DWORD winmode=0, winshare=0, wincreat=0, winattr=0;
	
	file nullFile;
	nullFile.error(EINVAL);
	
	// flag checks - VERY basic
	// must be binary access
	if((mode & _O_BINARY)==0) 
		return nullFile;
	
	// must be any of _S_IREAD, _S_IWRITE
	if((mode & _O_CREAT) && (createAttr & ~(_S_IREAD | _S_IWRITE)))
		return nullFile;
	
	if(mode & _O_CREAT)	{
		if((mode & _O_EXCL) > 0)
			wincreat|=CREATE_NEW;
		else
			wincreat|=OPEN_ALWAYS;
	}
	else
		wincreat|=OPEN_EXISTING;
		
	if((mode & O_WRONLY) > 0)
		winmode|=GENERIC_WRITE;
	else
		winmode|=GENERIC_READ;
	if((mode & _O_RDWR) > 0)
		winmode|=GENERIC_READ | GENERIC_WRITE;
	
	if((share & _SH_DENYRD) == 0)
		winshare|=FILE_SHARE_READ;
	if((share & _SH_DENYWR) == 0)
		winshare|=FILE_SHARE_WRITE;
	if((share & _SH_DENYNO) > 0)
		winshare|=FILE_SHARE_READ | FILE_SHARE_WRITE;
	if((share & _SH_DENYRW) > 0)
		winshare=0;
	
	// read only attr
	if(((createAttr & _S_IREAD) > 0) && ((createAttr & _S_IWRITE) == 0))
		winattr|=FILE_ATTRIBUTE_READONLY;
	
	HANDLE h=CreateFileA(name, winmode, winshare, NULL, wincreat, winattr, 0);
	file f(h);
	f.werror();
	return f;
}

io64::file::size io64::file::read(void *buffer, const size& size)
{
	error(0);
	DWORD readed;
	ReadFile(handle(), buffer, (DWORD)size, &readed, 0);
	werror();
	return readed;
}

io64::file::position io64::file::seek(const position& offset, int origin)
{
	//                         SEEK_SET    SEEK_CUR      SEEK_END
	static DWORD win_way[] = { FILE_BEGIN, FILE_CURRENT, FILE_END };

	error(0);
	
	LARGE_INTEGER of, newof;
	
	of.QuadPart=offset;
	
	BOOL res=SetFilePointerEx(handle(), of, &newof, win_way[origin]);
	werror();
	return res ? newof.QuadPart : -1;
}

io64::file::position io64::file::tell()
{
	LARGE_INTEGER off, newof;
	
	error(0);
	
	off.QuadPart=0;
	BOOL res=SetFilePointerEx(handle(), off, &newof, FILE_CURRENT);
	werror();
	return res ? newof.QuadPart : -1;
}

bool io64::file::setSize(const size& size)
{
	error(0);
	if(seek((position)size, SEEK_SET)==size){
		bool res=SetEndOfFile(handle())!=0;
		werror();
		return res;
	}
	else
		return false;
}

io64::file::size io64::file::write(const void *b, const size& size)
{
	DWORD written;
	WriteFile(handle(), b, (DWORD)size, &written, 0);
	werror();
	return written;
}

io64::file::size io64::file::getSize()
{
	LARGE_INTEGER s;
	BOOL res=GetFileSizeEx(handle(), &s);
	werror();
	return res ? s.QuadPart : -1;
}

bool io64::file::flush()
{
	BOOL res=FlushFileBuffers(handle());
	werror();
	return res!=0;
}

bool io64::file::eof()
{
	position current=tell();
	if(current==seek(0, SEEK_END))
		return true;
	else{
		seek(current, SEEK_SET);
		return false;
	}
}

bool io64::file::fstat(stat *stat)
{
	error(0);
	if(!valid()) return false;
	
	FILETIME CreationTime, AccessTime, WriteTime;
	
	if(!GetFileTime(handle(), &CreationTime, &AccessTime, &WriteTime)){
		error(EINVAL);
		return false;
	}
	wintime2ctime(&CreationTime, &stat->ctime);
	wintime2ctime(&AccessTime, &stat->atime);
	wintime2ctime(&WriteTime, &stat->mtime);
	
	stat->size=getSize();
	
	return true;
};
