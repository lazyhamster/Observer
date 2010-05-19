#ifndef CAT_INCLUDED
#define CAT_INCLUDED

#include <io.h>
#include "common/ext_list.h"
#include "common/strutils.h"
#include "common.h"
#include "file_io.h"
#include "libwdmatch/libwdmatch.h"

struct filebuffer;
//---------------------------------------------------------------------------------
struct x2catentry
{
	char *pszFileName;
	io64::file::position offset;
	io64::file::size size;
	
	filebuffer *buffer;
	
	x2catentry() { pszFileName=0; offset=0; size=0; buffer=0; }
	~x2catentry() { delete[] pszFileName; }
};
//---------------------------------------------------------------------------------
class x2catbuffer : public ext::list<x2catentry *> 
{
	private:
		char *m_pszFileName;
		char *m_pszDATName;
		io64::file m_hDATFile;
		io64::file m_hCATFile;
		int m_nError;
		int m_nRefCount;
		
		inline char * getDATName() const;
		int error(int err) { return (m_nError=err); }
		void fileerror(int ferr);
		
		iterator find(const char* pszName)
		{
			iterator it;
			for(it=begin(); it!=end(); ++it){
				if(_stricmp(it->pszFileName, pszName)==0)
					break;
			}
			return it;
		}
		
		io64::file::size dataSize()
		{
			io64::file::size size=0;
			for(iterator it=begin(); it!=end(); ++it){
				size+=it->size;
			}
			return size;
		}
		
		bool isValidFileName(const char *pszFileName);
		
	public:		
		x2catbuffer() { m_nRefCount=1; m_pszFileName=0; m_pszDATName=0; }
		~x2catbuffer() 
		{ 
			delete[] m_pszFileName; 
			delete[] m_pszDATName;
			m_hDATFile.close();
			m_hCATFile.close();
			for(iterator it=begin(); it!=end(); ++it){
				delete (*it);
			}
		}
		
		const char * fileName() const { return m_pszFileName; }
		const char * fileNameDAT() const { return m_pszDATName; }
		
		int error() const { return m_nError; }
		
		int refcount() const { return m_nRefCount; }
		int addref() { return ++m_nRefCount; }
		int release() 
		{ 
			if(--m_nRefCount==0){
				delete this;
				return 0;
			}
			else
				return m_nRefCount;
		}
		
		x2catentry * findFile(const char* pszName)
		{
			iterator it;
			for(it=begin(); it!=end(); ++it){
				if(_stricmp(it->pszFileName, pszName)==0)
					break;
			}
			return (it==end() ? NULL : *it);
		}
		
		bool open(const char *pszName);
		
		filebuffer * loadFile(const char *pszFile, int fileType);
		filebuffer * loadFile(x2catentry *entry, int fileType);
		void closeFile(filebuffer *buff);
		
		int getFileCompressionType(const char *pszFileName);
		bool fileStat(const char *pszFileName, X2FILEINFO *info);
		
		iterator findFirstFile(const char *pattern);
		iterator findNextFile(iterator it, const char *pattern);
};
//---------------------------------------------------------------------------------
class x2catalog
{
	private:
		x2catbuffer *m_buff;
		
	public:		
		x2catbuffer * buffer() const { return m_buff; }
		void buffer(x2catbuffer *buff) { if(m_buff) m_buff->release(); m_buff=buff; buff->addref(); }
		
		x2catalog() { m_buff=NULL; }
		x2catalog(x2catbuffer *buff) { m_buff=buff; buff->addref(); }
		
		~x2catalog() { if(m_buff) m_buff->release(); }
		
		x2catbuffer::iterator findFirstFile(const char *pattern) { return m_buff->findFirstFile(pattern); }
		x2catbuffer::iterator findNextFile(x2catbuffer::iterator it, const char *pattern) { return m_buff->findNextFile(it, pattern); }
};
//---------------------------------------------------------------------------------
inline char * x2catbuffer::getDATName() const
{
	char *base=ExtractFileName(fileName(), true);
	char *datname=strcat2(2, base, ".dat");
	delete[] base;
	return datname;
}
//---------------------------------------------------------------------------------

#endif // !defined(CAT_INCLUDED)