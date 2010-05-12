#ifndef X2FILES_INCLUDED
#define X2FILES_INCLUDED

#include <time.h>
#include "cat.h"
#include "x2fd.h"
#include "common/gzip.h"
#include "file_io.h"

#define TREATASFILE(buffer) ((buffer->type & filebuffer::ISFILE) && (buffer->type & filebuffer::ISPLAIN))

//---------------------------------------------------------------------------------
struct filebuffer
{	
	private:
		int m_nRefCount;
		int m_lockw;
		int m_lockr;
		x2catbuffer *m_cat;
		size_t m_allocated;
		size_t m_size;
		size_t m_binarysize;
		byte *m_data;
		bool m_bDirty;
		time_t m_mtime;
		int m_nErrCode;
		 
		static const size_t GROWBY = 102400;	// 100 KB
		
		void allocated(size_t newsize) { m_allocated=newsize; }
		io64::file::size writeBuffer(byte *data, const io64::file::size& size, io64::file::position offset);
		int error(int errcode) { return (m_nErrCode=errcode); }
		
		bool openFilePlain(const char *pszName, int nAccess, int nCreateDisposition);
		bool openFileCompressed(const char *pszName, int nAccess, int nCreateDisposition, int compressionType);
		
	public:
		static const int ISFILE = 1;
		static const int ISPLAIN = 2;
		static const int ISPCK = 4;
		static const int ISDEFLATE = 8;
		
		io64::file file;
		char *pszName;
		int type;
		
		filebuffer() 
		{ 
			m_cat=NULL; m_data=NULL; m_size=0; m_binarysize=0; m_nRefCount=1; 
			pszName=0; m_lockw=0; m_lockr=0; type=0; m_allocated=0; m_bDirty=0; m_mtime=-1;
			m_nErrCode=0;
		}
		
		~filebuffer() 
		{ 
			delete[] pszName; 
			delete[] m_data;
			if(m_cat) m_cat->release();
			file.close(); 
		}
		
		int error() const { return m_nErrCode; }
		
		size_t allocated() const { return m_allocated; }
		size_t size() const { return m_size; }
		size_t binarysize() const { return m_binarysize; }
		void binarysize(size_t newsize) { m_binarysize=newsize; }
		io64::file::size filesize();
		io64::file::size read(void *buffer, const io64::file::size& size, io64::file::position offset);
		bool save();
		
		byte * data() const { return m_data; }
		void data(byte *buffer, size_t buffsize, size_t datasize) 
		{ 
			delete[] m_data; 
			m_data=buffer; m_allocated=buffsize; m_size=datasize;
		}
		
		const byte * end() const { return data() + allocated(); }
		
		time_t mtime() const { return m_mtime; }
		void mtime(time_t modtime) { m_mtime=modtime; }
		
		int locked_w() const { return m_lockw; }
		int lock_w() { return ++m_lockw; }
		int unlock_w() { return (m_lockw ? --m_lockw : 0); }
		
		int locked_r() const { return m_lockr; }
		int lock_r() { return ++m_lockr; }
		int unlock_r() { return (m_lockr ? --m_lockr : 0); }
		
		int refcount() const { return m_nRefCount; }
		int addref() { return ++m_nRefCount; }
		int release() 
		{ 
			if(--m_nRefCount==0) {
				delete this; 
				return 0;
			}
			else
				return m_nRefCount;
		}
		
		bool dirty() const { return m_bDirty; }
		void dirty(bool val) { m_bDirty=val; }
		
		x2catbuffer* cat() const { return m_cat; }
		void cat(x2catbuffer *buff) { buff->addref(); if(m_cat) m_cat->release(); m_cat=buff; }
		
		io64::file::size write(byte *pData, const io64::file::size& size, io64::file::position offset);
		bool allocate(size_t newsize);
		
		bool convert(int nNewFileType);
	
		bool openFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType);
		
		static int fileTypeToBufferType(int fileType)
		{
			int res;
			if(fileType == X2FD_FILETYPE_PCK)
				res = ISPCK;
			else if(fileType == X2FD_FILETYPE_DEFLATE)
				res = ISDEFLATE;
			else
				res = ISPLAIN;
			return res;
		}
		
		static int bufferTypeToFileType(int bufferType)
		{
			int res;
			if((bufferType & ISPCK) > 0)
				res = X2FD_FILETYPE_PCK;
			else if((bufferType & ISDEFLATE) > 0)
				res = X2FD_FILETYPE_DEFLATE;
			else
				res = X2FD_FILETYPE_PLAIN;
			return res;
		}
};

struct xfile
{
	private:
		filebuffer *m_buffer;
		int m_mode;	// read/write
		int m_nErrCode;
		
		void error(int errcode) { m_nErrCode=errcode; } 
		
		io64::file::position m_offset;
		
		void offset(const io64::file::position& off) { m_offset=off; }
		
	public:
		/* 
			it's pretty unsafe to store the dataptr as the underlying 
			data pointer (in filebuff) my be reallocated (but it shouldn't happen due to r/w locks)
			
			I should rewrite this to use offset instead...
		*/
		//byte *dataptr; 
		
		xfile() { m_buffer=0; m_mode=0; m_offset=0; m_nErrCode=0; }
		~xfile() 
		{ 
			if(m_buffer){
				if(mode()==X2FD_WRITE) m_buffer->unlock_w();
				m_buffer->release(); 
			}
		}
		
		io64::file::position offset() const { return m_offset; }
		
		int mode() { return m_mode; }
		int mode(int m) 
		{
			if(m_mode==m) return m;
			int old=m_mode; 
			m_mode=m; 
			if(m_buffer){
				if(m==X2FD_WRITE)
					m_buffer->lock_w();
				else 
					m_buffer->unlock_w();
			}
			return old; 
		}
		
		int error() const { return m_nErrCode; }
		
		void mtime(time_t newmtime) { m_buffer->mtime(newmtime); }
		time_t mtime() const { return m_buffer->mtime(); }
		io64::file::size size() const { return m_buffer->size(); }
		io64::file::size binarysize() const { return m_buffer->binarysize(); }
		io64::file::size filesize() { return m_buffer->filesize(); }
		io64::file::size read(void *buffer, const io64::file::size& size) 
		{ 
			io64::file::size r=m_buffer->read(buffer, size, offset());
			if(r!=-1) 
				offset(offset() + r);
			error(m_buffer->error());
			return r;
		}
		
		bool eof() const;
		io64::file::position seek(const io64::file::offset& offset, int origin);
		io64::file::position tell() const;
		io64::file::size write(const void *data, const io64::file::size& count);
		bool seteof();
		bool save() 
		{ 
			if((mode() & X2FD_WRITE)==0){
				error(X2FD_E_FILE_BADMODE);
				return false;
			}
			bool bRes=m_buffer->save(); 
			error(m_buffer->error()); 
			return bRes; 
		}
		
		bool dirty() const { return m_buffer->dirty(); }
		
		byte* data() const { return m_buffer->data(); }
		
		filebuffer * buffer() const { return m_buffer; }
		void buffer(filebuffer *b) 
		{ 
			if(m_buffer==b) return;
			offset(0);
			if(m_buffer) {
				if(mode()==X2FD_WRITE)
					m_buffer->unlock_w();
					
				m_buffer->unlock_r();
				m_buffer->release();
			}
			m_buffer=b; 
			if(m_buffer){
				if(mode()==X2FD_WRITE) m_buffer->lock_w();
				m_buffer->lock_r();
				m_buffer->addref();
			}
		}
		
		bool convert(int nNewFileType)
		{
			bool bRes;
				
			if(mode() != X2FD_WRITE) {
				error(X2FD_E_FILE_BADMODE);
				return false;
			}
			
			io64::file::position pos = tell();	
			
			if((bRes=m_buffer->convert(nNewFileType))==false)
				error(m_buffer->error());
			else
				seek((io64::file::offset)pos, X2FD_SEEK_SET);
			
			return bRes;
		}
};
//---------------------------------------------------------------------------------

#endif // !defined(X2FILES_INCLUDED)