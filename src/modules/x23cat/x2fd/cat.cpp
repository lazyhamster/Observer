#include "StdAfx.h"
#include "cat.h"
#include "files.h"
#include "catpck.h"
#include "common/strutils.h"
#include "common/gzip.h"
#include "common/string_builder.h"
//---------------------------------------------------------------------------------
void x2catbuffer::fileerror(int ferr)
{  
	int e;
	switch(ferr){
		case EACCES:
			e=X2FD_E_FILE_ACCESS;
			break;
		case EEXIST:
			e=X2FD_E_FILE_EXIST;
			break;
		case ENOENT:
			e=X2FD_E_FILE_NOENTRY;
			break;
		default:
			e=X2FD_E_FILE_ERROR;
			break;
	}
	error(e);
}
//---------------------------------------------------------------------------------
bool x2catbuffer::open(const char *pszName)
{
	io64::file::size size, filesize;
	bool bRes=true;
	
	error(0);

	strcreate(m_pszFileName, pszName);
	m_hCATFile=io64::file::open(pszName, _O_RDWR | _O_BINARY, 0, _SH_DENYRW);
	if(!m_hCATFile.valid()) {
		fileerror(m_hCATFile.error());
		return false;
	}
	filesize=m_hCATFile.getSize();
	if(filesize==0){
		m_hCATFile.close();
		error(X2FD_E_FILE_EMPTY);
		return false;
	}
	// buffer size - must be multiple of 5 - see DecryptCAT for reason
	size=filesize + ((filesize % 5U) ? 5U - (filesize % 5U) : 0);
	//byte *buffer=new byte[size + 1];
	byte *buffer=new byte[(size_t)size + 1];
	buffer[filesize]=0;
	m_hCATFile.read(buffer, filesize);
	DecryptCAT(buffer, filesize);
	
	memrep(buffer, 13, 0, (size_t)filesize, 0);
	memrep(buffer, 10, 0, (size_t)filesize, 0);

	x2catentry *info;	
	char *pos, *old, *mid, *end;
	bool first=true;
	io64::file::size offset=0;
	pos=(char*)buffer;
	end = pos + filesize;
	do{
		old=pos;
		pos=(char*)memchr(pos, 0, end - pos);
		if(pos){
			if(first){
				strcreate(m_pszDATName, old);
				first=false;
			}
			else{
				mid=strrchr(old, ' ');
				if(mid==NULL){
					bRes=false;
					break;
				}
				mid[0]=0;
				info=new x2catentry();
				strcreate(info->pszFileName, old);
				strrep(info->pszFileName, '/', '\\');
				info->offset=offset;
				info->size=atoi(++mid);
				push_back(info);
				offset+=info->size;
			}
			for( ; *pos==0 && (pos < end); pos++); // skip zeroes
		}
	}
	while(pos && (pos < end));
	
	delete[] buffer;
	
	if(bRes==false)
		error(X2FD_E_FILE_BADDATA);
	else{
		// we must open the corresponding DAT file as well
		
		// do not use stored dat name because it is always 01.dat even for 02.cat and 03.cat
		char *path=ExtractFilePath(pszName);
		/*char *name=strcat2(3, path, "\\", fileNameDAT());
		
		m_hDATFile=fopen(name, "rb");
		delete[] name;
		if(m_hDATFile==NULL){ */
			char *datname=getDATName();
			char *name=0;
			if(path) {
				// don't remove the parenthesis in this if block - strcat2 is a macro!
				name=strcat2(3, path, "\\", datname);
			}
			else{
				strcreate(name, datname); 
			}
			delete[] datname;
			m_hDATFile=io64::file::open(name, _O_RDWR | _O_BINARY, 0, _SH_DENYRW);
			delete[] name;
		//}
		delete[] path;
		bRes=m_hDATFile.valid();
		if(!bRes){
			if(m_hDATFile.error()==ENOENT)
				error(X2FD_E_CAT_NODAT);
			else
				fileerror(m_hDATFile.error());
		}
	}
	return bRes;
}
//---------------------------------------------------------------------------------
filebuffer * x2catbuffer::loadFile(const char *pszFile, int fileType)
{
	x2catentry *entry=findFile(pszFile);
	if(entry==NULL){
		error(X2FD_E_CAT_NOENTRY);
		return NULL;
	}
	else
		return loadFile(entry, fileType);
}
//---------------------------------------------------------------------------------
filebuffer * x2catbuffer::loadFile(x2catentry *entry, int fileType)
{
	filebuffer *buff=NULL;
	byte *outdata; 
	io64::file::size outsize;
	time_t mtime=-1; // -1 mean "not set"
	
	buff=new filebuffer();
	if(entry->size==0){
		outdata=0;
		outsize=0;
		if(fileType==X2FD_FILETYPE_AUTO) fileType=X2FD_FILETYPE_PCK;
		buff->type|=fileType;
	}
	else{
		byte *data=new byte[(size_t)entry->size];
		m_hDATFile.seek((io64::file::offset)entry->offset, SEEK_SET);
		if(m_hDATFile.read(data, entry->size)!=entry->size){
			error(X2FD_E_CAT_INVALIDSIZE);
			return NULL;
		}
		
		DecryptDAT(data, entry->size);
		
		if(fileType==X2FD_FILETYPE_AUTO)
			fileType = GetBufferCompressionType(data, entry->size);
		
		// plain file
		if(fileType==X2FD_FILETYPE_PLAIN){
			outdata=data;
			outsize=entry->size;
			buff->type|=filebuffer::ISPLAIN;
		}
		// compressed
		else {
			error(DecompressBuffer(data, entry->size, fileType, &outdata, &outsize, &mtime));
			bool bErr=(error()!=0 && error()!=X2FD_E_GZ_FLAGS);
			delete[] data;
			if(bErr){
				delete buff;
				buff=NULL;
			}
			else
				buff->type|=filebuffer::fileTypeToBufferType(fileType);
		}
	}
	if(buff){
		entry->buffer=buff;
		buff->cat(this);
		buff->data(outdata, (size_t)outsize, (size_t)outsize);
		buff->mtime(mtime);
		buff->binarysize((size_t)entry->size);			
		buff->pszName=strcat2(3, fileName(), "::", entry->pszFileName);
	}

	return buff;
}
//---------------------------------------------------------------------------------
void x2catbuffer::closeFile(filebuffer *buff)
{
	if(buff->cat()==this){
		for(iterator it=begin(); it!=end(); ++it){
			if(it->buffer==buff) {
				it->buffer=NULL;
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------------
#include <stdio.h>
void x2catbuffer::saveCAT()
{
	string_builder str;
	char b[20];
	// set the grow by for string high enough and set initial size to that of the 
	// current cat file
	str.growby(1000);
	str.reserve((string_builder::size_type)m_hCATFile.getSize());
	str << fileNameDAT() << '\n';
	
	/*char msg[200];
	sprintf(msg, "count in dat: %d", (int)size());
	MessageBox(0, msg, 0, 0);*/
	
	for(iterator it=begin(); it!=end(); ++it){
		_itoa((int)it->size, b, 10);
		str << it->pszFileName << ' ' << b << '\n';
	}
	
	// replace windows slash for unix
	memrep(const_cast<char*>(str.buffer()), '\\', '/', str.size());
	
	// the buffer size must be multiple of 5 - see DecryptCAT
	if(str.size() % 5 != 0)
		str.reserve((str.size() / 5 + 1) * 5);
	
	// DecryptCAT is bidirectional, so this will encrypt the CAT
	DecryptCAT(const_cast<byte*>(reinterpret_cast<const byte*>(str.buffer())), str.size());
	
	m_hCATFile.seek(0, SEEK_SET);
	m_hCATFile.write(str, str.size());
	m_hCATFile.setSize(m_hCATFile.tell());
	m_hCATFile.flush();
	dirty(false);
}
//---------------------------------------------------------------------------------
bool x2catbuffer::saveFile(filebuffer *buff)
{
	// - find the buffer
	// - check number of locks
	// - save that shit:
	//   - move all files below our file up and save our file at the end
	//   - modify the offset in cat entries and move modified file to the end
	char *pszCAT, *pszFile;
	gzwriter gz;
	char magic;
	byte *data; 
	io64::file::size datalen, speclen, sizeleft;
	 
	ParseCATPath(buff->pszName, &pszCAT, &pszFile);
	iterator pos=find(pszFile);
	if(pos==end()){
		error(X2FD_E_CAT_NOENTRY);
		return false;
	}
	
	x2catentry *entry=*pos;
	if(buff->locked_r() > 1){
		error(X2FD_E_FILE_LOCKED);
		return false;
	}
	
	// calculate size of data below our file 
	sizeleft=0;
	for(iterator it=pos + 1; it!=end(); ++it){
		sizeleft+=it->size;
	}
	if(buff->type & filebuffer::ISPLAIN){
		data=buff->data();
		datalen=buff->size();
		speclen=datalen;
	}
	// compressed
	else{
		// pack the file - if it fails the DAT remains unchanged
		if(error(CompressBuffer(buff->data(), buff->size(), buff->mtime(), filebuffer::bufferTypeToFileType(buff->type), &gz, &magic)) != 0) return false;
		data = gz.buffer();
		datalen = gz.size();
		speclen = datalen + (buff->type & filebuffer::ISPCK ? 1 : 0);
	}

	bool bNoMove = (speclen == entry->size);

	// there are some data after our file and size of old and new buffer differs
	if(sizeleft > 0 && bNoMove==false){
		byte *buff;
		// buffer size (max 20 MB)
		size_t size=(size_t)__min(sizeleft, 20480000);
		buff=new byte[size];
		if(buff==0) {
			error(X2FD_E_MALLOC);
			return false;
		}
		
		// now move the data after our file in the DAT
		
		io64::file::size rpos, wpos, s, sl=sizeleft;
		
		rpos=entry->offset + entry->size;
		wpos=entry->offset;
		do{
			s=__min(size, sl);
			m_hDATFile.seek((io64::file::offset)rpos, SEEK_SET);
			m_hDATFile.read(buff, s);
			m_hDATFile.seek((io64::file::offset)wpos, SEEK_SET);
			m_hDATFile.write(buff, s);
			rpos+=s;
			wpos+=s;
			sl-=s;
		}
		while(sl > 0);
		
		delete[] buff;
		
		// shift the offsets
		for(iterator it=pos + 1; it!=end(); ++it){
			it->offset-=entry->size;
		}
		// set our new offset
		entry->offset=wpos;
	}
	else{
		// if there are no data below our file, just seek to the beginning of the entry
		m_hDATFile.seek((io64::file::offset)entry->offset, SEEK_SET);
	}
	
	// move cat entry to the end
	if(bNoMove==false && pos!=end() - 1){
		erase(pos);	// 'pos' is the iterator, 'entry' is its value
		push_back(entry);
	}
	// set dirty so destructor will save the modified CAT
	dirty(true);
	
	// set the new size of modified entry
	entry->size=datalen;
	
	// save the data - all data are XORed with 33H
	if(buff->type & filebuffer::ISPCK){
		entry->size++; // increase size of file due to magic
		magic^=0x33;
		m_hDATFile.write(&magic, 1);
	}
	byte *dpos, *dend=data + datalen;
	for(dpos=data; dpos < dend; dpos++){
		(*dpos)^=0x33;
	}
	m_hDATFile.write(data, datalen);
		
	buff->dirty(false);
	
	return true;
}
//---------------------------------------------------------------------------------
bool x2catbuffer::isValidFileName(const char *pszFileName)
{
	if(pszFileName==0) return false;
	for(const char *ch=pszFileName; *ch!=0; ch++){
		if(*ch=='/' || *ch==':' || *ch=='*' || *ch=='<' || *ch=='>' || *ch=='|')
			return false;
	}
	return true;
}
//---------------------------------------------------------------------------------
filebuffer * x2catbuffer::createFile(const char *pszFile, int fileType)
{
	if(isValidFileName(pszFile)==false){
		error(X2FD_E_CAT_INVALIDFILENAME);
		return NULL;
	}
	if(fileType != X2FD_FILETYPE_PCK && fileType != X2FD_FILETYPE_PLAIN && fileType != X2FD_FILETYPE_DEFLATE){
		error(X2FD_E_BAD_FLAGS);
		return NULL;
	}
	
	x2catentry *entry=new x2catentry();
	filebuffer *buff=new filebuffer();
	
	buff->pszName=strcat2(3, fileName(), "::", pszFile);
	buff->dirty(true);
	buff->type |= filebuffer::fileTypeToBufferType(fileType);
	
	buff->cat(this);
	buff->mtime(time(0));
	
	strcreate(entry->pszFileName, pszFile);
	entry->buffer=buff;
	
	for(iterator it=begin(); it!=end(); ++it){
		entry->offset+=it->size;
	}
	
	push_back(entry);
	dirty(true);
	return buff;
}
//---------------------------------------------------------------------------------
bool x2catbuffer::create(const char *pszName)
{
	bool bRes=false;
	
	strcreate(m_pszFileName, pszName);
	m_pszDATName=getDATName();
	m_hCATFile=io64::file::open(pszName, _O_CREAT | _O_EXCL | _O_BINARY | _O_RDWR);
	if(!m_hCATFile.valid()){
		fileerror(m_hCATFile.error());
		return false;
	}
	char *path=ExtractFilePath(pszName);
	char *pszDATPath=strcat2(3, path, "\\", m_pszDATName);
	m_hDATFile=io64::file::open(pszDATPath, _O_CREAT | _O_EXCL | _O_BINARY | _O_RDWR);
	if(!m_hDATFile.valid()){
		fileerror(m_hDATFile.error());
	}
	else {
		// save the cat right now so it will be valid even if the lib will crash
		// or something
		saveCAT();
		bRes=true;
	}
	delete[] pszDATPath; 
	delete[] path;
	return bRes;
}
//---------------------------------------------------------------------------------
bool x2catbuffer::deleteFile(const char *pszFileName)
{
	iterator entry=find(pszFileName);
	if(entry==end()){
		error(X2FD_E_CAT_NOENTRY);
		return false;
	}
	
	// shrink the DAT and delete the entry
	
	// calculate size of data below our file
	io64::file::size sizeleft=0;
	for(iterator it=entry + 1; it!=end(); ++it){
		sizeleft+=it->size;
	}
	
	io64::file::size rpos, wpos, s;
	
	byte *buff;
	// buffer size (max 20 MB)
	size_t size=(size_t)__min(sizeleft, 20480000);
	buff=new byte[size];
	if(buff==0) {
		error(X2FD_E_MALLOC);
		return false;
	}
	
	// mark oursef dirty so saveCAT will be called upon close
	dirty(true);
	
	rpos=entry->offset + entry->size;
	wpos=entry->offset;
	do{
		s=__min(size, sizeleft);
		m_hDATFile.seek((io64::file::offset)rpos, SEEK_SET);
		m_hDATFile.read(buff, s);
		m_hDATFile.seek((io64::file::offset)wpos, SEEK_SET);
		m_hDATFile.write(buff, s);
		rpos+=s;
		wpos+=s;
		sizeleft-=s;
	}
	while(sizeleft > 0);
	
	m_hDATFile.setSize(m_hDATFile.tell());
	
	delete[] buff;
	
	// shift the offsets
	for(iterator it=entry + 1; it!=end(); ++it){
		it->offset-=entry->size;
	}
	
	delete *entry;
	erase(entry);
	
	return true;
}
//---------------------------------------------------------------------------------
bool x2catbuffer::renameFile(const char *pszFileName, const char *pszNewName)
{
	x2catentry *source, *dest;
	
	if(isValidFileName(pszNewName)==false){
		error(X2FD_E_CAT_INVALIDFILENAME);
		return false;
	}
	
	source = findFile(pszFileName);
	dest = findFile(pszNewName);
	if(source == 0){
		error(X2FD_E_CAT_NOENTRY);
		return false;
	}
	// allow rename to the "same" name with possible case change: "hallo" -> "Hallo"
	if(dest != 0 && _stricmp(pszFileName, pszNewName) != 0){
		error(X2FD_E_FILE_EXIST);
		return false;
	}
	strcreate(source->pszFileName, pszNewName);
	dirty(true);
	return true;
}
//---------------------------------------------------------------------------------
int x2catbuffer::getFileCompressionType(const char *pszFileName)
{
	int nRes;
	unsigned char data[3];
	iterator it=find(pszFileName);
	
	error(0);
	
	if(it==end())
		error(X2FD_E_CAT_NOENTRY);
	else{
		if(it->size >= 3){
			m_hDATFile.seek((io64::file::offset)it->offset, SEEK_SET);
			m_hDATFile.read(data, 3);
			nRes = GetBufferCompressionType(data, 3) == X2FD_FILETYPE_PCK;
		}
	}
	return nRes;
}
//---------------------------------------------------------------------------------
bool x2catbuffer::fileStat(const char *pszFileName, X2FILEINFO *info)
{
	iterator it=find(pszFileName);
	if(it==end()){
		error(X2FD_E_CAT_NOENTRY);
		return false;
	}
	char *name=strcat2(3, m_pszFileName, "::", it->pszFileName);
	m_hDATFile.seek((io64::file::offset)it->offset, SEEK_SET);
	FileStat(true, name, m_hDATFile, it->size, info);
	delete[] name;
	return true;
}
//---------------------------------------------------------------------------------
x2catbuffer::iterator x2catbuffer::findFirstFile(const char *pattern)
{
	iterator it;
	for(it=begin(); it!=end(); ++it){
		if(libwdmatch_match_i(pattern, it->pszFileName)) break;
	}
	return it;
}
//---------------------------------------------------------------------------------
x2catbuffer::iterator x2catbuffer::findNextFile(x2catbuffer::iterator it, const char *pattern)
{
	for( ; it!=end(); ++it){
		if(libwdmatch_match_i(pattern, it->pszFileName)) break;
	}
	return it;
}
//---------------------------------------------------------------------------------

