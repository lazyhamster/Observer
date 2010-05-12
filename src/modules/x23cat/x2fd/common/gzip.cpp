#include "StdAfx.h"
#include <memory.h>
#include <zlib.h>

#include "gzip.h"

#pragma comment(lib, "../../common/zlib.lib")

#define flagsvalid(f) ((f & (GZ_F_RES1 | GZ_F_RES2 | GZ_F_RES3))==0)
//---------------------------------------------------------------------------------
bool gzreader::openFile(const char *pszName)
{
	FILE *file=fopen(pszName, "rb");
	bool res=false;
	if(file){
		fseek(file, 0, SEEK_END);
		size_t size=ftell(file);
		fseek(file, 0, SEEK_SET);
		if(m_fileBuffer) delete[] m_fileBuffer;
		m_fileBuffer=new unsigned char[size];
		fread(m_fileBuffer, 1, size, file);
		fclose(file);
		
		res=unpack(m_fileBuffer , size );
	}
	else
		error(GZ_E_FILE);
		
	return res;
}
//---------------------------------------------------------------------------------
bool gzreader::unpack(unsigned char *buffer, size_t size)
{
	unsigned char *ptr=buffer;
	
	error(0);
	
	if(size < header_size) {
		error(GZ_E_TOOSMALL);
		return false;
	}
	
	if(buffer[0]!=0x1F || buffer[1]!=0x8B) {
		error(GZ_E_HEADER);
		return false;
	}
	
	memcpy(&m_mtime, buffer + GZ_MTIME, sizeof(time_t));
	
	m_buffer=buffer;
	
	ptr=buffer + GZ_OS + 1;
	if(flags() & GZ_F_EXTRA){
		size_t xlen=*((short int*)(ptr));
		ptr+=xlen;
	}
	
	try{
		if(flags() & GZ_F_FILENAME){
			m_origname=(char*)(ptr);
			ptr+=strlen(m_origname) + 1;
		}
		if(flags() & GZ_F_COMMENT){
			m_comment=(char*)(ptr);
			ptr+=strlen(m_comment) + 1;
		}
	}
	catch(...){
		error(GZ_E_INTERNAL_ERROR);
		return false;
	}
	if(flags() & GZ_F_HCRC){
		m_hcrc=(unsigned short int*)(ptr);
		ptr+=2;
	}
	m_crc=(size_t*)(buffer + size - 8);
	m_unpackedsize=(size_t*)(buffer + size - 4);
	
	m_stream=ptr;
	m_streamsize= size-(ptr-buffer) - 8;
	
	if(compression()!=GZ_CM_DEFLATE){
		error(GZ_E_COMPRESSION);
		return false;
	}
	
	if(!flagsvalid(flags())) error(GZ_E_FLAGS);
	
	m_outdata=new value_type[unpackedsize()];
	
	z_stream zs;
	memset(&zs, 0, sizeof(z_stream));
	zs.avail_in=(uInt)streamsize();
	zs.next_in=stream();
	zs.avail_out=(uInt)unpackedsize();
	zs.next_out=m_outdata;

	int ret;	
	try{
		ret=inflateInit2(&zs, -15);
		ret=inflate(&zs, Z_FINISH);
	}
	catch(...){
		inflateEnd(&zs);
		error(GZ_E_INTERNAL_ERROR);
		return false;
	}
	m_outsize=zs.total_out;
	inflateEnd(&zs);
	
	bool bRes;
	if(bRes=(ret==Z_STREAM_END)){ 
		unsigned long crc=crc32(0, NULL, 0);
		crc=crc32(crc, outdata(), (uInt)outsize());
		if((bRes=((size_t)crc==this->crc()))==false) 
			error(GZ_E_CRC);
	}
	else
		error(GZ_E_DEFLATE);
		
	return bRes;
}
//---------------------------------------------------------------------------------
bool gzwriter::allocate(size_t newsize)
{
	value_type *old=m_buffer;
	size_t ptroffset;
	ptroffset=ptr() - m_buffer;
	m_buffer=new value_type[newsize];
	if(m_buffer==NULL) {
		delete[] old;
		return false;
	}
	memcpy(m_buffer, old, __min(newsize, allocated()));
	delete[] old;
	m_allocated=newsize;
	m_ptr=m_buffer + __min(ptroffset, newsize);
	return true;
}
//---------------------------------------------------------------------------------
void gzwriter::put(const char * text)
{
	if(text==NULL) return;
	size_t len=strlen(text) + 1;
	while(avail() < len) {
		if(!allocate(allocated() + growby)) return;
	}
	memcpy(ptr(), text, len);
	m_ptr+=len;
	m_size+=len;
}
//---------------------------------------------------------------------------------
template <class T> 
void gzwriter::put(const T& val)
{
	while(sizeof(val) > avail()){
		if(!allocate(allocated() + growby)) return;
	}
	memcpy(ptr(), &val, sizeof(val));
	m_ptr+=sizeof(val);
	m_size+=sizeof(val);
}
//---------------------------------------------------------------------------------
bool gzwriter::pack(value_type *buffer, size_t size)
{
	z_stream zs;
	int flags=0;
	
	error(0);
	
	if(m_pszComment && strlen(m_pszComment) > 0) flags|=GZ_F_COMMENT;
	if(m_pszFileName && strlen(m_pszFileName) > 0) flags|=GZ_F_FILENAME;	
	
	put((value_type)GZ_ID1);
	put((value_type)GZ_ID2);
	put((value_type)8);				// this is hack, should be determined at runtime
	put((value_type)flags);
	put(m_mtime);
	put((value_type)0);
	put((value_type)11);
	if(flags & GZ_F_FILENAME) put((const char*)m_pszFileName);
	if(flags & GZ_F_COMMENT) put((const char*)m_pszComment);
	
	memset(&zs, 0, sizeof(zs));
	zs.next_in=buffer;
	zs.avail_in=(uInt)size;

	int ret;
	unsigned long ubound;
	ret=deflateInit2(&zs, compression(), Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if(ret!=Z_OK) {
		error(GZ_E_DEFLATE);
		return false;
	}
	ubound=deflateBound(&zs, (uLong)size);
	 
	if(avail() < ubound) allocate(allocated() + ubound);
	
	zs.next_out=ptr();
	zs.avail_out=(uInt)avail();
	while((ret=deflate(&zs, Z_FINISH))==Z_OK){
		allocate(allocated() + growby);
		zs.next_out=ptr() + zs.total_out;
		zs.avail_out=(uInt)(avail() - zs.total_out);
	}
	m_size+=(size_t)zs.total_out;
	m_ptr=m_buffer + m_size;

	deflateEnd(&zs);

	unsigned long crc=crc32(0, NULL, 0);
	crc=crc32(crc, buffer, (uInt)size);
	put(crc);
	put(size);	
	if(ret!=Z_STREAM_END) error(GZ_E_DEFLATE);
	return ret==Z_STREAM_END;
}
//---------------------------------------------------------------------------------