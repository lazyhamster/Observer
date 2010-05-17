#include "StdAfx.h"
#include <io.h>
#include "files.h"
#include "x2fd.h"
#include "catpck.h"
#include "local_error.h"
//#include "../common/filetime.h"
//---------------------------------------------------------------------------------
io64::file::size filebuffer::write(byte *pData, const io64::file::size& size, io64::file::position offset)
{
	io64::file::size count;
	error(0);
	if(TREATASFILE(this)){
		count=file.write(pData, size);
	}
	else{
		count = writeBuffer(pData, size, offset);
	}
	if(count >= 0) mtime(time(0));	// update mtime
	dirty(dirty() || count > 0);
	return count;
}
//---------------------------------------------------------------------------------
io64::file::size filebuffer::writeBuffer(byte *pData, const io64::file::size& size, io64::file::position offset)
{
	// resize the buffer if neccessary
	if((allocated() - offset) < size){
		io64::file::size newsize = this->size() + size;
		io64::file::size s = (newsize / GROWBY) + (newsize % GROWBY ? 1 : 0 + 1);
		if(allocate((size_t)s * GROWBY) == false) return -1;
	}
	memcpy(m_data + offset, pData, (size_t)size);
	m_size = __max(m_size, (size_t)(offset + size));
	return size;
}
//---------------------------------------------------------------------------------
// grow the buffer (preserve written data)
bool filebuffer::allocate(size_t newsize)
{
	if(newsize <= allocated()) {
		m_size=newsize;
		return true;
	}
	byte *old=m_data;
	size_t count;
	m_data=new byte[newsize];
	if(m_data==NULL) {
		m_data=old;
		error(X2FD_E_MALLOC);
		return false;
	}
	count=__min(size(), newsize);
	allocated(newsize);
	m_size=count;
	memcpy(m_data, old, count);
	delete[] old;
	return true;
}
//---------------------------------------------------------------------------------
bool filebuffer::convert(int nNewFileType)
{
	if(nNewFileType != X2FD_FILETYPE_PCK && nNewFileType != X2FD_FILETYPE_PLAIN && nNewFileType != X2FD_FILETYPE_DEFLATE){
		error(X2FD_E_BAD_FLAGS);
		return false;
	}
	if(nNewFileType==X2FD_FILETYPE_PCK && (type & filebuffer::ISPCK)) 
		return true;
	else if(nNewFileType==X2FD_FILETYPE_PLAIN && (type & filebuffer::ISPLAIN)) 
		return true;
	else if(nNewFileType==X2FD_FILETYPE_DEFLATE && (type & filebuffer::ISDEFLATE)) 
		return true;
	
	int mask = 0;
	if((type & ISPCK) > 0) mask = ISPCK;
	else if((type & ISDEFLATE) > 0) mask = ISDEFLATE;
	else if((type & ISPLAIN) > 0) mask = ISPLAIN;
	
	if((type & ISFILE) > 0){
		// no plain -> pck to deflate or vice versa - just change the type
		if(nNewFileType != X2FD_FILETYPE_PLAIN && (type & ISPLAIN) == 0) {
			type &= ~mask;
		}
		// plain -> compressed
		else if((type & ISPLAIN) > 0){
			io64::file::size size=file.getSize();
			unsigned char *buff=new unsigned char[(size_t)size];
			if(buff==NULL){
				error(X2FD_E_MALLOC);
				return false;
			}
			file.read(buff, size);
			data(buff, (size_t)size, (size_t)size);
			type &= ~mask;
		}
		// compressed -> plain
		else {
			file.seek(0, SEEK_SET);
			file.write(data(), size());
			delete[] m_data;
			m_data=NULL;
			m_allocated=0;
			type &= mask;
		}
	}
	// cat
	else {
		type &= ~mask;
	}
	type |= fileTypeToBufferType(nNewFileType);
	mtime(time(0));
	dirty(true);
	return true;
}
//---------------------------------------------------------------------------------
io64::file::size filebuffer::filesize()
{
	io64::file::size s;
	if(TREATASFILE(this))
		s=file.getSize();
	else
		s=size();
		
	return s;
}
//---------------------------------------------------------------------------------
io64::file::size filebuffer::read(void *buffer, const io64::file::size& size, io64::file::position offset)
{
	io64::file::size readed;
	if(TREATASFILE(this)){
		readed=file.read(buffer, size);
		if(readed!=size) error(fileerror(file.error()));
	}
	else{
		if(offset > this->allocated()) // offset is beyond eof (and if allocated()==0)
			return 0;
		
		readed=size;
		if(size > (this->allocated() - offset)) // shring the size so we won't read past eof
			readed=this->allocated() - offset;
		memcpy(buffer, data() + offset, (size_t)readed);
	}
	return readed;
}
//---------------------------------------------------------------------------------
#include <sys\utime.h>
bool filebuffer::save()
{
	bool bRes = false;
	if(m_cat){
		error(X2FD_E_FILE_ACCESS);
	}
	else if(type & ISFILE){
		if(type & ISPLAIN)
			bRes = file.flush();
		else {
			error(X2FD_E_FILE_BADDATA);
			bRes=(error()==0);
		}
		time_t m = mtime();
		file.setTime(&m, &m, &m);
	}
	return bRes;
}
//---------------------------------------------------------------------------------
bool filebuffer::openFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType)
{	
	bool bRes;
	
	error(0);
	
	if(nFileType==X2FD_FILETYPE_AUTO)
		nFileType = GetFileCompressionType(pszName);
	
	if(nFileType != X2FD_FILETYPE_PLAIN)
		bRes = openFileCompressed(pszName, nAccess, nCreateDisposition, nFileType);
	else
		bRes = openFilePlain(pszName, nAccess, nCreateDisposition);
	
	return bRes;
}
//---------------------------------------------------------------------------------
bool filebuffer::openFilePlain(const char *pszName, int nAccess, int nCreateDisposition)
{
	int omode=_O_BINARY, share = _SH_DENYNO;
	bool bRes=true;
	
	if(nCreateDisposition==X2FD_CREATE_NEW) 
		omode|=_O_CREAT;
	if(nAccess==X2FD_READ){
		omode|=_O_RDONLY;
		share=_SH_DENYWR;
	}
	else if(nAccess==X2FD_WRITE){
		omode|=_O_RDWR;
		share=_SH_DENYRW;
	}
	file=io64::file::open(pszName, omode, _S_IREAD | _S_IWRITE, share);
	if(!file.valid()){
		error(fileerror(file.error()));
		bRes=false;
	} 
	else{
		strcreate(this->pszName, pszName);
		type|=filebuffer::ISFILE;
		type|=filebuffer::ISPLAIN;
		io64::file::stat fs;
		file.fstat(&fs);
		mtime(fs.mtime);
		binarysize((size_t)fs.size);
		
	}
	return bRes;
}
//---------------------------------------------------------------------------------
bool filebuffer::openFileCompressed(const char *pszName, int nAccess, int nCreateDisposition, int compressionType)
{
	bool bEmptyFile=false, bRes=true;
	io64::file::size filesize;
	
	int omode=_O_BINARY, share = _SH_DENYNO;
	if(nCreateDisposition == X2FD_CREATE_NEW) 
		omode|=_O_CREAT;
	if(nAccess==X2FD_READ) {
		omode|=_O_RDONLY;
		share=_SH_DENYWR;
	}
	else if(nAccess==X2FD_WRITE) {
		omode|=_O_RDWR;
		share=_SH_DENYRW;
	}
	file=io64::file::open(pszName, omode, _S_IREAD | _S_IWRITE, share);
	if(!file.valid()){
		error(fileerror(file.error()));
		bRes=false;
	}
	bEmptyFile=((filesize=file.getSize()) == 0);
	if(bRes){
		// unpack file only if it's not empty
		if(bEmptyFile==false){
			byte *buffer;
			io64::file::size size;
			time_t mtime;
			error(DecompressFile(file, compressionType, &buffer, &size, &mtime));
			if(bRes=(error() == 0 || error() == X2FD_E_GZ_FLAGS)){
				data(buffer, (size_t)size, (size_t)size);
				this->mtime(mtime);
				// close the file - it won't be needed in the future
				if(nAccess!=X2FD_WRITE) {
					file.close();
				}
			}
		}
		else{
			io64::file::stat fs;
			file.fstat(&fs);
			mtime(fs.mtime);
			dirty(true);
		}
		if(bRes){
			strcreate(this->pszName, pszName);
			if(compressionType == X2FD_FILETYPE_PCK)
				type|=filebuffer::ISPCK;
			else if(compressionType == X2FD_FILETYPE_DEFLATE)
				type|=filebuffer::ISDEFLATE;
			type|=filebuffer::ISFILE;
			binarysize((size_t)filesize);
		}
	}
	return bRes;
}
//---------------------------------------------------------------------------------

bool xfile::eof()  const
{
	if(TREATASFILE(m_buffer))
		return m_buffer->file.eof();
	else
		return offset() == size();
}
//---------------------------------------------------------------------------------
io64::file::position xfile::seek(const io64::file::offset& offset, int origin)
{
	io64::file::position newoff=-1;
	
	if(TREATASFILE(m_buffer)){
		int way;
		switch(origin){
			case X2FD_SEEK_CUR:
				way=SEEK_CUR;
				break;
			case X2FD_SEEK_END:
				way=SEEK_END;
				break;
			case X2FD_SEEK_SET:
				way=SEEK_SET;
				break;
			default:
				error(X2FD_E_BAD_SEEK_SPEC);
				return -1;
		}
		newoff=m_buffer->file.seek(offset, way);
		if(newoff==-1)
			error(X2FD_E_SEEK);
	}
	else{
		io64::file::offset p;
		switch(origin){
			case X2FD_SEEK_END:
				p=(io64::file::offset)size();
				break;
			case X2FD_SEEK_SET:
				p=0;
				break;
			case X2FD_SEEK_CUR:
				p = (io64::file::offset)(io64::file::position)this->offset();
				break;
			default:
				error(X2FD_E_BAD_SEEK_SPEC);
				return -1;
		}
		p+=offset;
		
		if(p < 0)
			error(X2FD_E_SEEK);
		else{
			this->offset((size_t)p);
			newoff=(io64::file::position)p; // seek on buffer always succeeds - later write may however fail
		}
	}
	return newoff;
}
//---------------------------------------------------------------------------------
io64::file::position xfile::tell() const
{
	io64::file::position pos;
	if(TREATASFILE(m_buffer))
		pos=m_buffer->file.tell();
	
	else
		pos=offset();
		
	return pos;
}
//---------------------------------------------------------------------------------
io64::file::size xfile::write(const void *data, const io64::file::size& count)
{
	if((mode() & X2FD_WRITE)==0){
		error(X2FD_E_FILE_BADMODE);
		return -1;
	}
	io64::file::size s=m_buffer->write((byte*)data, count, offset());
	if(s==-1) 
		error(m_buffer->error());
	else
		offset(offset() + (size_t)s);
	return s;
}
//---------------------------------------------------------------------------------
bool xfile::seteof()
{
	bool bRes;
	if((mode() & X2FD_WRITE)==0){
		error(X2FD_E_FILE_BADMODE);
		return false;
	}
	if(TREATASFILE(m_buffer)){
		bRes=m_buffer->file.setSize(m_buffer->file.tell());
	}
	else{
		bRes=m_buffer->allocate((size_t)offset());
		if(bRes==false)
			error(m_buffer->error());
	}
	return bRes;
}
//---------------------------------------------------------------------------------
