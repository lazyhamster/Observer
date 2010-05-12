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
X2FDEXPORT(X2FDLONG) X2FD_ReadFile(X2FILE hFile, void *buffer, X2FDULONG size)
{
	xfile *f=getfile(hFile);
	X2FDLONG r=-1;
	clrerr();
	if(f==NULL)
		error(X2FD_E_HANDLE);
	else{
		r=(X2FDLONG)f->read(buffer, size);
		if(r==-1) error(f->error());
	}
	return r;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_EOF(X2FILE hFile)
{
	xfile *f=getfile(hFile);
	clrerr();
	if(f)
		return f->eof();
	else{
		error(X2FD_E_HANDLE);
		return 0;
	}
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FDULONG) X2FD_SeekFile(X2FILE hFile, int offset, int origin)
{
	xfile *f=getfile(hFile);
	X2FDULONG newoff = (X2FDULONG)-1;
	clrerr();
	if(f==NULL)
		error(X2FD_E_HANDLE);
	else{
		newoff=(X2FDULONG)f->seek(offset, origin);
		if(newoff==-1) error(f->error());
	}
	return newoff;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FDULONG) X2FD_FileTell(X2FILE hFile)
{
	xfile *f=getfile(hFile);
	X2FDULONG pos = (X2FDULONG)-1;
	clrerr();
	if(f==NULL)
		error(X2FD_E_HANDLE);
	else
		pos=(X2FDULONG)f->tell();
	return pos;
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
X2FDEXPORT(X2FDLONG) X2FD_WriteFile(X2FILE hFile, const void *pData, X2FDULONG nCount)
{
	xfile *f = getfile(hFile);
	io64::file::size res;

	clrerr();
	if(f == NULL) {
		error(X2FD_E_HANDLE);
		res = -1;
	}
	else{
		res = f->write(pData, nCount);
		if(res == -1) error(f->error());
	}
	return (X2FDLONG)res;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_SetEndOfFile(X2FILE hFile)
{
	bool bRes;
	xfile *f = getfile(hFile);
	if(f == NULL) {
		error(X2FD_E_HANDLE);
		return false;
	}

	bRes = f->seteof();
	if(bRes == false) error(f->error());
	return bRes;
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
X2FDEXPORT(int) X2FD_FileStat(const char *pszFileName, X2FILEINFO *pInfo)
{
	char *pszCat, *pszFile, *pszFullFileName;
	bool bRes = false;

	pszFullFileName = _fullpath(0, pszFileName, 0);

	ParseCATPath(pszFullFileName, &pszCat, &pszFile);
	if(pszCat){
		x2catbuffer *cat = _OpenCatalog(pszCat, X2FD_OPEN_EXISTING);
		if(cat){
			bRes = cat->fileStat(pszFile, pInfo);
			if(bRes == false) error(cat->error());
			ReleaseCatalog(cat);
		}
	}
	else{
		io64::file fd;
		filebuffer *f = findBuffer(pszFullFileName);
		if(f)
			fd = f->file;
		else
			fd = io64::file::open(pszFullFileName, _O_BINARY | _O_RDONLY);

		if(!fd.valid())
			error(fileerror(fd.error()));
		else{
			FileStat(false, pszFileName, fd, fd.getSize(), pInfo);
			bRes = true;
		} 
		if(f)
			f->release();
		else
			fd.close();
	}
	delete[] pszCat;
	delete[] pszFile;
	delete[] pszFullFileName;
	return bRes;
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
X2FDEXPORT(int) X2FD_DeleteFile(const char *pszFileName)
{
	char *pszFullName=_fullpath(0, pszFileName, 0);
	bool bRes=false;

	clrerr();
	if(pszFullName==0){
		error(X2FD_E_ERROR);
		return false;
	}

	filebuffer *b=findBuffer(pszFullName);
	// file is opened - error
	if(b){
		b->release();
		error(X2FD_E_FILE_LOCKED);
	}
	else{
		const char *pos=strstr(pszFullName, "::");
		// normal file
		if(pos==NULL){
			bRes=(::remove(pszFullName)==0);
			if(bRes==false)
				error(fileerror(errno));
		}
		// file from CAT
		else{
			char *catname, *filename;
			ParseCATPath(pszFullName, &catname, &filename);
			x2catbuffer *cat=_OpenCatalog(catname, X2FD_OPEN_EXISTING);
			if(cat){
				bRes=cat->deleteFile(filename);
				if(bRes==false) error(cat->error());
				ReleaseCatalog(cat);
			}
			delete[] catname;
			delete[] filename;
		}
	}
	delete[] pszFullName;
	return bRes;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(int) X2FD_DeleteCatalog(const char *pszFileName)
{
	bool bRes=false;

	clrerr();
	char *pszFullName=_fullpath(0, pszFileName, 0);
	if(pszFileName==0){
		error(X2FD_E_ERROR);
		return false;
	}
	x2catbuffer *cat=findCatBuff(pszFullName);
	if(cat){
		error(X2FD_E_FILE_LOCKED);
		cat->release();
	}
	else{
		char *path, *datname, *datpath;
		path=ExtractFilePath(pszFullName);
		datname=ExtractFileName(pszFullName, true);
		datpath=strcat2(4, path, "\\", datname, ".dat");

		bRes=(::remove(pszFullName)==0);
		if(bRes==false)
			error(fileerror(errno));
		else{
			bRes=(::remove(datpath)==0);
			if(bRes==false)
				error(fileerror(errno));
		}
		delete[] path;
		delete[] datpath;
		delete[] datname;
	}
	delete[] pszFullName;
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
X2FDEXPORT(int) X2FD_MoveFile(const char *pszFileName, const char *pszNewName)
{
	char *fullname, *fullname2;
	char *catname, *filename, *catname2, *filename2;
	x2catbuffer *srccat = 0, *targetcat = 0;
	bool bRes = false;

	clrerr();

	// normal files
	if((strstr(pszFileName, "::") == 0) && (strstr(pszNewName, "::") == 0)){
		if((bRes = (::rename(pszFileName, pszNewName)==0))==false) error(fileerror(errno));
	}
	else if((strstr(pszFileName, "::") != 0) || (strstr(pszNewName, "::") != 0)){
		fullname = _fullpath(0, pszFileName, 0);
		fullname2 = _fullpath(0, pszNewName, 0);

		ParseCATPath(fullname, &catname, &filename);
		ParseCATPath(fullname2, &catname2, &filename2);

		bool bSameCat = (_stricmp(catname, catname2) == 0);
		srccat = _OpenCatalog(catname, X2FD_OPEN_EXISTING);
		if(srccat){
			targetcat = _OpenCatalog(catname2, X2FD_CREATE_NEW);

			// move inside same catalog -> simple rename
			if(bSameCat && srccat){
				bRes = srccat->renameFile(filename, filename2);
				if(bRes == false) error(srccat->error());
			}
			// move across catalogs
			else if(srccat && targetcat){
				X2FILE sourcefile, destfile;
				sourcefile = OpenFileCAT(pszFileName, X2FD_READ, X2FD_OPEN_EXISTING, X2FD_FILETYPE_PLAIN);
				if(sourcefile) {
					destfile = X2FD_OpenFile(pszNewName, X2FD_WRITE, X2FD_CREATE_NEW, X2FD_FILETYPE_PLAIN);
					if(destfile){
						size_t size = X2FD_FileSize(sourcefile);
						if(size >= 0) {
							unsigned char *data = new unsigned char[size];
							if(size != (size_t)X2FD_ReadFile(sourcefile, data, size))
								error("MoveFile: Error reading file.");
							else {
								if(size != (size_t)X2FD_WriteFile(destfile, data, size))
									error("MoveFile: Error writing file.");
								else
									bRes=true;
							}
							delete[] data;
						}
						X2FD_CloseFile(destfile);
					}
					X2FD_CloseFile(sourcefile);
					// we can delete only after the source file has been closed
					if(bRes) bRes = (X2FD_DeleteFile(pszFileName)!=0);
				}
			}
		}
		if(srccat) ReleaseCatalog(srccat);
		if(targetcat) ReleaseCatalog(targetcat);
		delete[] catname;
		delete[] filename;
		delete[] fullname;
		delete[] catname2;
		delete[] filename2;
		delete[] fullname2;
	}
	else
		error("MoveFile supports only files within catalogs.");

	return bRes;
}
//---------------------------------------------------------------------------------
X2FDEXPORT(X2FILE) X2FD_OpenFileConvert(const char *pszName, int nCreateDisposition, int nFileType, int nConvertToFileType)
{
	clrerr();
	X2FILE hFile=X2FD_OpenFile(pszName, X2FD_WRITE, nCreateDisposition, nFileType);
	if(hFile){
		xfile *file=(xfile*) hFile;
		if(file->convert(nConvertToFileType)==false){
			error(file->error());
			X2FD_CloseFile(hFile);
			hFile=0;
		}
	}
	return hFile;
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
X2FDEXPORT(double) X2FD_TimeStampToOLEDateTime(unsigned long TimeStamp)
{
	// timestamp: number of seconds from midnight, 1 January, 1970
	// datetime: number of days from midnight, 30 December, 1899
	if(TimeStamp==-1) return 0;
	TimeStamp+=2209161600; // number of seconds between 30 December 1899 and 1 January 1970
	return ((double)TimeStamp)/3600/24; // convert number of sec. to number of days
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