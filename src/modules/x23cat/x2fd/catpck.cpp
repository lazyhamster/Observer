#include "StdAfx.h"
#include "catpck.h"
#include "local_error.h"
#include "file_io.h"
#include "x2fd.h"
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
int GetFileCompressionType(const char *pszName)
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
int DecompressFile(const char *pszName, int compressionMethod, unsigned char **out_data, io64::file::size *out_size, time_t *mtime)
{
	io64::file file=io64::file::open(pszName, _O_RDONLY | _O_BINARY);
	int nRes;
	if(!file.valid())
		nRes=fileerror(file.error());
	else{
		nRes=DecompressFile(file, compressionMethod, out_data, out_size, mtime);
	}
	file.close();
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
		for(size_t i=0; i < in_size; i++){
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
int CompressBufferToFile(unsigned char *buffer, const io64::file::size& size, io64::file& file, time_t mtime, int compressionType)
{
	/*
	int nRes=0;
	gzwriter gz;
	gz.compression(GZ_COMP_MAX);
	gz.mtime(mtime);

	if(gz.pack(buffer, (size_t)size)==false)
		nRes=TranslateGZError(gz.error());
	else{
		char magic = (char)clock(), m;
		m=magic ^ 0xC8;
		file.seek(0, SEEK_SET);
		file.write(&m, 1);
		gzwriter::value_type *ptr, *end;
		ptr=gz.buffer(); end=gz.buffer() + gz.size();
		for( ; ptr < end; ptr++){
			(*ptr)^=magic;
		}
		file.write(gz.buffer(), gz.size());
		file.setSize((gz.size() + 1));
	}
	return nRes;
	*/
	gzwriter gz;
	char magic;
	int nRes = CompressBuffer(buffer, size, mtime, compressionType, &gz, &magic);
	
	file.seek(0, SEEK_SET);
	if(compressionType == X2FD_FILETYPE_PCK)
		file.write(&magic, 1);
	file.write(gz.buffer(), gz.size());
	file.setSize(gz.size() + (compressionType == X2FD_FILETYPE_PCK ? 1 : 0));
	
	return nRes;
}
//---------------------------------------------------------------------------------
int CompressBuffer(unsigned char *buffer, const io64::file::size& size, time_t mtime, int compressionType, gzwriter *gz, char *magic)
{
	int nRes=0;
	gz->compression(GZ_COMP_MAX);
	gz->mtime(mtime);
	if(gz->pack(buffer, (size_t)size) == false)
		nRes = TranslateGZError(gz->error());
	else{
		if(compressionType == X2FD_FILETYPE_PCK){
			char m = (char)clock();
			*magic = m ^ 0xC8;
			gzwriter::value_type *ptr, *end;
			ptr=gz->buffer(); end=gz->buffer() + gz->size();
			for( ; ptr < end; ptr++){
				(*ptr)^=m;
			}
		}
		else
			*magic = 0;
	}
	return nRes;
}
//---------------------------------------------------------------------------------
void FileStat(bool bIsCat, const char *pszFileName, io64::file& fd, const io64::file::size& size, X2FILEINFO *pInfo)
{
	unsigned char data[4];
	int mtime = 0, logsize = 0, i;
	io64::file::size begin = fd.tell();
	int type = X2FD_FILETYPE_PLAIN;
	
	if(size >= 3){
		fd.read(data, 3);
		if(bIsCat){
			for(i=0; i < 3; i++){
				data[i]^=0x33;
			}
		}
		type = GetBufferCompressionType(data, 3);
		if(type != X2FD_FILETYPE_PLAIN){
			char magic = 0;
			int headersize = 0;
			if(type == X2FD_FILETYPE_PCK) {
				magic = data[0] ^ 0xC8;
				headersize = 3;
			}
			else
				headersize = 2;
			fd.seek(begin + headersize + 2, SEEK_SET);
			fd.read(data, 4);
			for(i=0; i < 4; i++){
				if(bIsCat) data[i] ^= 0x33;
				if(magic != 0)
					data[i] ^= magic;
			}
			memcpy(&mtime, data, 4);
			
			fd.seek(begin + size - 4, SEEK_SET);
			fd.read(data, 4);
			for(int i=0; i < 4; i++){
				if(bIsCat) data[i]^=0x33;
				if(magic != 0)
					data[i] ^= magic;
			}
			memcpy(&logsize, data, 4);
		}
		fd.seek((io64::file::offset)begin, SEEK_SET); // return the file pointer to its original position
	}
	if(type != X2FD_FILETYPE_PLAIN){
		pInfo->size = logsize;
		pInfo->flags = type == X2FD_FILETYPE_PCK ? X2FD_FI_PCK : X2FD_FI_DEFLATE;
		pInfo->mtime = mtime;
	}
	else{
		pInfo->size=(unsigned long)size;
		pInfo->flags=X2FD_FI_PLAIN;
		if(bIsCat==false){
			io64::file::stat s;
			fd.fstat(&s);
			pInfo->mtime=(int)s.mtime;
		}
		else
			pInfo->mtime=-1;
	}
	strncpy(pInfo->szFileName, pszFileName, sizeof(pInfo->szFileName));
	pInfo->BinarySize=(unsigned long)size;
	
	if(bIsCat==false) 
		pInfo->flags|=X2FD_FI_FILE;
}
//---------------------------------------------------------------------------------