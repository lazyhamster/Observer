#include "StdAfx.h"
#include "common/ext_list.h"
#include "common/strutils.h"
#include "x2fd.h"
#include "cat.h"
#include "files.h"
#include "catpck.h"
#include "time2.h"
#include "local_error.h"

#define getfile(handle) (xfile*) handle
#define getcat(handle) (x2catalog*) handle

typedef ext::list<x2catbuffer*> catlist;
typedef ext::list<filebuffer*> bufferlist;

catlist g_catlist;
bufferlist g_bufflist;

struct CATFINDFILEINFO
{
	x2catbuffer *buffer;
	x2catbuffer::iterator it;

	char *pattern;
};
//---------------------------------------------------------------------------------
#ifdef _WINDLL
int finalCloseFiles()
{
	for(bufferlist::iterator it=g_bufflist.begin(); it!=g_bufflist.end(); ++it){
		if(it->dirty()) 
			it->save();
		delete *it;
	}
	int i=(int) g_bufflist.size();
	g_bufflist.clear();
	return i;
}
//---------------------------------------------------------------------------------
int finalCloseCatalogs()
{
	for(catlist::iterator it=g_catlist.begin(); it!=g_catlist.end(); ++it){
		delete *it;
	}
	int i=(int) g_catlist.size();
	g_bufflist.clear();
	return i;
}
//---------------------------------------------------------------------------------
#endif // defined(_WINDLL)
//---------------------------------------------------------------------------------
bool checkBuffMode(filebuffer *b, int mode)
{
	if(mode!=X2FD_READ && mode!=X2FD_WRITE) return false;
	return !((mode==X2FD_WRITE) || (b->locked_w() > 0 && (mode==X2FD_READ)));
}
//---------------------------------------------------------------------------------
filebuffer* findBuffer(const char *pszName)
{
	bufferlist::iterator it;

	for(it=g_bufflist.begin(); it!=g_bufflist.end(); ++it)	{
		if(_stricmp(it->pszName, pszName)==0)
			break;
	}
	if(it!=g_bufflist.end()){
		it->addref();
		return *it;
	}
	else
		return NULL;
}
//---------------------------------------------------------------------------------
x2catbuffer * findCatBuff(const char *pszName)
{
	catlist::iterator it;
	for(it=g_catlist.begin(); it!=g_catlist.end(); ++it){
		if(_stricmp(it->fileName(), pszName)==0)
			break;
	}
	if(it!=g_catlist.end()){
		it->addref();
		return *it;
	}
	else
		return NULL;
}
//---------------------------------------------------------------------------------
x2catbuffer * _OpenCatalog(const char *pszName, int nCreateDisposition)
{
	x2catbuffer *cbuff;
	bool bRes;
	cbuff=findCatBuff(pszName);
	if(cbuff==NULL){
		cbuff=new x2catbuffer();
		bRes=cbuff->open(pszName);
		if(bRes==false && nCreateDisposition==X2FD_CREATE_NEW && cbuff->error()==X2FD_E_FILE_NOENTRY)
			bRes=cbuff->create(pszName);

		if(bRes==false){
			error(cbuff->error());
			cbuff->release();
			cbuff=NULL;
		}
		else
			g_catlist.push_back(cbuff);
	}
	return cbuff;
}
//---------------------------------------------------------------------------------
int ReleaseCatalog(x2catbuffer *catalog)
{
	if(catalog->refcount()==1)
		g_catlist.erase(g_catlist.find(catalog));

	return catalog->release();
}
//---------------------------------------------------------------------------------
X2FILE OpenFileCAT(const char *pszName, int nAccess, int nCreateDisposition, int nFileType)
{
	char *pszCATName=0, *pszFile=0;

	const char *pos = strstr(pszName, "::");
	if(pos==NULL) return 0;

	// look if buffer is already open
	filebuffer *buff=findBuffer(pszName);
	if(buff!=NULL){
		if(checkBuffMode(buff, nAccess)==false){
			buff->release();
			buff=NULL;
			error(X2FD_E_FILE_BADMODE);
		}
	}
	// we must open it
	else{
		ParseCATPath(pszName, &pszCATName, &pszFile);
		// find or open catalog
		x2catbuffer *catbuff=_OpenCatalog(pszCATName, X2FD_OPEN_EXISTING);
		if(catbuff!=NULL){
			buff=catbuff->loadFile(pszFile, nFileType);
			if(buff==NULL && nCreateDisposition==X2FD_CREATE_NEW && catbuff->error()==X2FD_E_CAT_NOENTRY)
				buff=catbuff->createFile(pszFile, nFileType==X2FD_FILETYPE_AUTO ? X2FD_FILETYPE_PLAIN : nFileType);

			if(buff)
				g_bufflist.push_back(buff);
			else
				error(catbuff->error());

			if(catbuff->release()==0) g_catlist.erase(g_catlist.find(catbuff));
		}
	}

	xfile *f;
	if(buff){
		f=new xfile();
		f->buffer(buff);
		f->mode(nAccess);
		buff->release();
	}
	else
		f=NULL;

	delete[] pszCATName;
	delete[] pszFile;

	return (f==NULL ? NULL : (X2FILE) f);
}
//---------------------------------------------------------------------------------
X2FILE OpenFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType)
{
	filebuffer *buff;

	clrerr();
	if(nFileType == X2FD_FILETYPE_AUTO)
		nFileType = GetFileCompressionType(pszName);

	buff=findBuffer(pszName);
	if(buff){
		// check the filemode
		if(checkBuffMode(buff, nAccess)==false){
			error(X2FD_E_FILE_BADMODE);
			buff->release();
			return 0;
		}
	}
	// plain files - we will always open new filebuffer
	if(nFileType==X2FD_FILETYPE_PLAIN){
		if(buff) buff->release();
		buff=NULL;
	}

	if(buff==NULL){
		buff=new filebuffer();
		bool bRes=buff->openFile(pszName, nAccess, nCreateDisposition, nFileType);
		error(buff->error()); // always set the error
		if(bRes==false){
			buff->release();
			buff=NULL;
		}
		else
			g_bufflist.push_back(buff);
	}

	xfile *f;
	if(buff){
		f=new xfile();
		f->buffer(buff);
		f->mode(nAccess);
		buff->release();
	}
	else
		f=NULL;

	return (f==NULL ? NULL : (X2FILE) f);
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FILE) X2FD_OpenFile(const char *pszName, int nAccess, int nCreateDisposition, int nFileType)
{
	clrerr();
	// check the flags so we can rely on them later
	if(nAccess != X2FD_READ && nAccess != X2FD_WRITE){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}
	if(nCreateDisposition != X2FD_CREATE_NEW && nCreateDisposition != X2FD_OPEN_EXISTING){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}
	if(nFileType != X2FD_FILETYPE_PCK && nFileType != X2FD_FILETYPE_DEFLATE && nFileType != X2FD_FILETYPE_PLAIN && nFileType != X2FD_FILETYPE_AUTO){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}
	if(nCreateDisposition == X2FD_CREATE_NEW && nAccess != X2FD_WRITE){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}

	// get the normalized full name
	char *pszFullName=_fullpath(0, pszName, 0);
	if(pszFullName==0){
		error(X2FD_E_ERROR);
		return 0;
	}

	X2FILE f;
	if(strstr(pszFullName, "::")==NULL)
		f=OpenFile(pszFullName, nAccess, nCreateDisposition, nFileType);
	else
		f=OpenFileCAT(pszFullName, nAccess, nCreateDisposition, nFileType);

	delete[] pszFullName;
	return f;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FDLONG) X2FD_FileSize(X2FILE hFile)
{
	xfile *f=getfile(hFile);
	X2FDLONG s=-1;

	clrerr();
	if(!f)
		error(X2FD_E_HANDLE);
	else
		s=(X2FDLONG)f->filesize();

	return s;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_GetLastError()
{
	return error();
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FDULONG) X2FD_TranslateError(int nErrCode, char *pszBuffer, X2FDLONG nBuffSize, X2FDLONG *pNeeded)
{
	size_t m=0, errlen;
	const char *msg=errormsg(nErrCode);
	if(msg)
		errlen=strlen(msg);
	else
		errlen=0;

	if(pszBuffer && nBuffSize > 0){
		strncpy(pszBuffer, msg, m=__min((size_t)nBuffSize, errlen + 1));
		pszBuffer[m - 1]=0;
	}
	if(pNeeded) *pNeeded=errlen + 1;
	return m;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2CATALOG) X2FD_OpenCatalog(const char *pszName, int nCreateDisposition)
{
	x2catalog *cat=NULL;
	x2catbuffer *cbuff;

	clrerr();
	if(nCreateDisposition!=X2FD_CREATE_NEW && nCreateDisposition!=X2FD_OPEN_EXISTING){
		error(X2FD_E_BAD_FLAGS);
		return 0;
	}

	// convert to normalized fullname
	char *pszFullName=_fullpath(0, pszName, 0);
	if(pszFullName==0){
		error(X2FD_E_ERROR);
		return false;
	}

	cbuff=_OpenCatalog(pszFullName, nCreateDisposition);
	if(cbuff) {
		cat=new x2catalog(cbuff);
		cbuff->release();
	}
	delete[] pszFullName;
	return (X2CATALOG) cat;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_CloseCatalog(X2CATALOG hCat)
{
	x2catalog *cat=(x2catalog*) hCat;
	clrerr();
	if(cat!=NULL){
		if(cat->buffer()->refcount()==1){
			g_catlist.erase(g_catlist.find(cat->buffer()));
		}
		delete cat;
		return true;
	}
	else{
		error(X2FD_E_HANDLE);
		return false;
	}
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_CloseFile(X2FILE hFile)
{
	xfile *f = getfile(hFile);
	bool bRes = false;

	clrerr();
	if(f)	{
		// remove cat from list if it will be deleted
		if(f->buffer()->cat() && f->buffer()->cat()->refcount() == 1)
			g_catlist.erase(g_catlist.find(f->buffer()->cat()));

		// remove and save filebuffer if it will be deleted
		if(f->buffer()->refcount() == 1){
			g_bufflist.erase(g_bufflist.find(f->buffer()));

			if(f->dirty())
				bRes = f->save();
			else
				bRes = true;

			if(bRes == false)
				error(f->error());
		}
		delete f;
	}
	return bRes;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_FileStatByHandle(X2FILE hFile, X2FILEINFO *pInfo)
{
	xfile *f = getfile(hFile);
	if(f == NULL){
		error(X2FD_E_HANDLE);
		return 0;
	}

	pInfo->flags = f->buffer()->type;
	pInfo->mtime = f->buffer()->mtime();
	pInfo->size = (X2FDLONG)f->size();
	pInfo->BinarySize = (X2FDLONG)f->binarysize();
	strncpy(pInfo->szFileName, f->buffer()->pszName, sizeof(pInfo->szFileName));

	return 1;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_FileExists(const char *pszFileName)
{
	const char *pos = strstr(pszFileName, "::");
	bool bRes=false;
	// normal file
	if(pos==0){
		_finddata_t ffd;
		intptr_t h=_findfirst(pszFileName, &ffd);
		bRes=(h!=-1);
		_findclose(h);
	}
	// file from cat
	else{
		char *pszCAT, *pszFile;
		ParseCATPath(pszFileName, &pszCAT, &pszFile);
		x2catbuffer *cat=_OpenCatalog(pszCAT, X2FD_OPEN_EXISTING);
		if(cat){
			bRes=(cat->findFile(pszFile)!=NULL);
			if(cat->release()==0) g_catlist.erase(g_catlist.find(cat));
		}
		delete[] pszCAT;
		delete[] pszFile;
	}
	return bRes;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_GetFileCompressionType(const char *pszFileName)
{
	char *pszCat, *pszFile;
	int nRes = X2FD_FILETYPE_PLAIN;

	clrerr();

	ParseCATPath(pszFileName, &pszCat, &pszFile);

	if(pszCat==0)
		nRes = GetFileCompressionType(pszFileName);
	else{
		x2catbuffer *cat=_OpenCatalog(pszCat, X2FD_OPEN_EXISTING);
		if(cat){
			nRes = cat->getFileCompressionType(pszFile);
			error(cat->error());
			ReleaseCatalog(cat);
		}
	}
	delete[] pszCat;
	delete[] pszFile;
	return nRes;
}
//---------------------------------------------------------------------------------
// converts timestamp in GTM to local time using current TZ and daylight saving
// if applicable at given date
X2FDEXPORT(X2FDLONG) X2FD_TimeStampToLocalTimeStamp(X2FDLONG TimeStamp)
{
	if(TimeStamp==-1) return -1;

	return (X2FDLONG) TimeStampToLocalTimeStamp((time_t*)&TimeStamp);
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_SetFileTime(X2FILE hFile, X2FDLONG mtime)
{
	xfile *f=getfile(hFile);
	if(f==0){
		error(X2FD_E_HANDLE);
		return false;
	}
	f->mtime(mtime);
	return true;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FIND) X2FD_CatFindFirstFile(X2CATALOG hCat, const char *pszFileName, X2CATFILEINFO *pInfo)
{
	CATFINDFILEINFO *ffi=0;

	clrerr();

	x2catalog *c=getcat(hCat);
	if(c==0){
		error(X2FD_E_HANDLE);
		return false;
	}

	x2catbuffer::iterator it=c->findFirstFile(pszFileName);

	if(it!=c->buffer()->end()){
		ffi=new CATFINDFILEINFO();
		ffi->buffer=c->buffer();
		ffi->it=it;

		ffi->pattern=0;
		strcreate(ffi->pattern, pszFileName);

		pInfo->size=(size_t)it->size;
		strncpy(pInfo->szFileName, it->pszFileName, sizeof(pInfo->szFileName));
	}

	return (X2FIND)ffi;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_CatFindNextFile(X2FIND hFind, X2CATFILEINFO *pInfo)
{
	clrerr();

	CATFINDFILEINFO *ffi=(CATFINDFILEINFO*) hFind;
	if(ffi==NULL){
		error(X2FD_E_HANDLE);
		return false;
	}

	ffi->it=ffi->buffer->findNextFile(++(ffi->it), ffi->pattern);

	if(ffi->it!=ffi->buffer->end()){
		pInfo->size=(size_t)ffi->it->size;
		strncpy(pInfo->szFileName, ffi->it->pszFileName, sizeof(pInfo->szFileName));
	}
	return ffi->it!=ffi->buffer->end();
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_CatFindClose(X2FIND hFind)
{
	clrerr();
	CATFINDFILEINFO *ffi=(CATFINDFILEINFO*) hFind;
	if(ffi==NULL) {
		error(X2FD_E_HANDLE);
		return false;
	}
	delete[] ffi->pattern;
	delete ffi;
	return true;
}
//---------------------------------------------------------------------------------
/*
	copy data from one file to another
	uses some optimalizations

	both source and destination files are set to EOF after the copy
	Furthermore SetEndOfFile(destination) is called
*/
X2FDEXPORT(int) X2FD_CopyFile(X2FILE hSource, X2FILE hDestination)
{
	xfile *src, *dest;
	src=getfile(hSource);
	dest=getfile(hDestination);
	bool bRes;

	clrerr();

	if(src==NULL || dest==NULL){
		error(X2FD_E_HANDLE);
		return false;
	}
	if(dest->mode()!=X2FD_WRITE){
		error(X2FD_E_FILE_BADMODE);
		return false;
	}

	io64::file::size size;
	unsigned char *data;
	bool srcisnative = (src->buffer()->type & filebuffer::ISFILE) && (src->buffer()->type & filebuffer::ISPLAIN);
	bool destisnative = (dest->buffer()->type & filebuffer::ISFILE) && (dest->buffer()->type & filebuffer::ISPLAIN);

	if(srcisnative){
		size=src->filesize();
		data=new unsigned char[(size_t)size];
		src->seek(0, X2FD_SEEK_SET);
		if(src->read(data, size)!=size){
			error(src->error());
			delete[] data;
			return false;
		}
	}
	else{
		data=src->buffer()->data();
		size=src->size();
		src->seek(0, X2FD_SEEK_END);
	}

	if(srcisnative && destisnative==false){
		dest->buffer()->data(data, (size_t)size, (size_t)size);
		dest->seek(0, X2FD_SEEK_END);
		dest->buffer()->dirty(true);
		data=NULL;
		bRes=true;
	}
	else{
		dest->seek(0, X2FD_SEEK_SET);
		bRes=(dest->write(data, size)==size);
		if(bRes==false)
			error(dest->error());
	}
	
	// if no mtime is available, use current time
	time_t mtime = src->mtime();
	if(mtime == -1)
		time(&mtime);
	dest->mtime(mtime);
	dest->seteof();

	if(srcisnative)
		delete[] data;

	return bRes;
}
//---------------------------------------------------------------------------------