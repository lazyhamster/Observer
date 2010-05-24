#ifndef CAT_INCLUDED
#define CAT_INCLUDED

#include <io.h>
#include "common/ext_list.h"
#include "common/strutils.h"
#include "common.h"
#include "file_io.h"

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
		
		int error(int err) { return (m_nError=err); }
		void fileerror(int ferr);
		
	public:
		x2catbuffer() { m_pszFileName=0; m_pszDATName=0; }
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
		
		bool open(const char *pszName);
		
		filebuffer * loadFile(x2catentry *entry, int fileType);
};
//---------------------------------------------------------------------------------

#endif // !defined(CAT_INCLUDED)