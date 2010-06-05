#ifndef FILE_IO_INCLUDED
#define FILE_IO_INCLUDED

#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <errno.h>

#include <stdarg.h>

namespace io64
{

class file
{
	friend class io64;

	public:
		static const int EUNKNOWN=0xffff;

		class size
		{
			public:
				typedef __int64 size_type;
			private:
				size_type value;
			public:
				size() : value(0) {}

				size(__int64 i) : value(i) {}
				size(void *i) : value((intptr_t)i) {}
				size(size_t i) : value(i) {}
				size(unsigned long i) : value(i) {}
				size(int i) : value(i) {}

				operator __int64() const { return (__int64)value; }

				__int64 operator %(unsigned int i) { return value % i; }

				size& operator =(const size& s) { value=s.value; return *this; }

				size& operator+=(const size& s) { value+=s.value; return *this; }
				size& operator-=(const size& s) { value-=s.value; return *this; }

				size operator++(int) { size r=*this; ++value; return r; }

		};

		typedef size position;

		struct stat
		{
			time_t ctime, atime, mtime;
			file::size size;
		};

	private:
		HANDLE m_hFile;
		int m_error;

		HANDLE handle() const { return m_hFile; }

		static void ctime2wintime(const time_t& time, FILETIME *wintime);
		static void wintime2ctime(const FILETIME *wintime, time_t *time);

		void error(int code) { m_error=(code==0 && !valid() ? EBADF : code); }

		void werror();

	public:
		file() { m_hFile=INVALID_HANDLE_VALUE; error(0); }
		file(HANDLE h) { m_hFile=h; error(0); }
		file(const file& f) { m_error=f.m_error; m_hFile=f.m_hFile; }

		bool valid() const { return handle()!=INVALID_HANDLE_VALUE; }
		int error() const { return m_error; }

		position tell();
		position seek(const position& offset, int origin);

		bool setSize(const size& size);
		size getSize();

		size write(const void *b, const size& size);
		size read(void *buffer, const size& size);

		bool flush();
		bool eof();

		bool setTime(const time_t *ctime, const time_t *atime, const time_t *mtime);
		bool mtime(const time_t& time);
		bool atime(const time_t& time);
		bool ctime(const time_t& time);
		bool fstat(stat *stat);

		static file open(const wchar_t *name, int mode, int createAttr=_S_IREAD | _S_IWRITE, int share=_SH_DENYNO);
		bool close() { bool bRes=(CloseHandle(m_hFile)!=0); m_hFile=INVALID_HANDLE_VALUE; return bRes; }

};

}; // namespace io64

#endif