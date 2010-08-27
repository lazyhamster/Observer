#include "StdAfx.h"
#include "catpck.h"
#include "local_error.h"
#include "file_io.h"
//---------------------------------------------------------------------------------
/* size must be multiple of 5! */
void DecryptCAT(unsigned char *buffer, const io64::file::size& size)
{
	unsigned char magic[]={ 0xDB, 0xDC, 0xDD, 0xDE, 0xDF };
	
	unsigned char *ptr=buffer, *end=buffer + size;
	
	for(int i=0; ptr < end; ptr++, i++){
		int j=i % 5;
		*ptr^=magic[j];
		magic[j]+=5;
	}
}
//---------------------------------------------------------------------------------
// although you can skip this function and will still be able to 
// unpack PCKs from DATs, you will fail to load nonpacked data
// don't ask me why...
void DecryptDAT(unsigned char *buffer, const io64::file::size& size)
{
	for(unsigned char *pos=buffer, *end=buffer + size; pos < end; pos++){
		*pos^=0x33;
	}
}
//---------------------------------------------------------------------------------
int GetFileCompressionType(const wchar_t *pszName)
{
	unsigned char data[3];
	io64::file file=io64::file::open(pszName, _O_RDONLY | _O_BINARY);
	if(file.valid()){
		file.read(data, sizeof(data));
		file.close();
		return GetBufferCompressionType(data, sizeof(data));
	}
	else
		return X2FD_FILETYPE_PLAIN;
}
//---------------------------------------------------------------------------------
int GetBufferCompressionType(const unsigned char *data, const io64::file::size& size)
{
	int type = -1;
	if(size >= 3){
		unsigned char magic=data[0] ^ 0xC8;
		if((data[1] ^ magic)==0x1F && (data[2] ^ magic)==0x8B) 
			type = X2FD_FILETYPE_PCK;
	}
	if(type == -1){
		if(size >= 2 && (data[0] == 0x1F && data[1] == 0x8B))
			type = X2FD_FILETYPE_DEFLATE;
		else
			type = X2FD_FILETYPE_PLAIN;
	}	
	return type;
}
//---------------------------------------------------------------------------------
int DecompressFile(io64::file& file, int compressionMethod, unsigned char **out_data, io64::file::size *out_size, time_t  *mtime)
{
	int nRes;
	unsigned char *data;
	io64::file::size size=file.getSize();
	data=new unsigned char[(size_t)size];
	if(data==NULL)
		return X2FD_E_MALLOC;
	
	file.read(data, size);
	nRes = DecompressBuffer(data, size, compressionMethod, out_data, out_size, mtime);
	delete[] data;
	return nRes;
}
//---------------------------------------------------------------------------------
int DecompressBuffer(unsigned char *in_data, const io64::file::size& in_size, int compressionMethod, unsigned char **out_data, io64::file::size *out_size, time_t *mtime)
{
	if(in_size <= 0)
		return X2FD_E_FILE_EMPTY;
	
	unsigned char magic=in_data[0] ^ 0xC8;
	int nRes=0;
	int offset = 0;
	
	if(compressionMethod == X2FD_FILETYPE_PCK){
		offset = 1;
		for(__int64 i=0; i < in_size; i++){
			in_data[i] ^= magic;
		}
	}
	gzreader gz;
	bool bRes=gz.unpack(in_data + offset, (size_t)in_size - offset);
	nRes = TranslateGZError(gz.error()); // set the error regardless of the return value
	
	if(bRes==false){
		gz.flush();
		*out_data=NULL;
		*out_size=0;
		*mtime=0;
	}
	else{
		*out_data=gz.outdata();
		*out_size=gz.outsize();
		*mtime=gz.mtime();
	}
	return nRes;
}
//---------------------------------------------------------------------------------