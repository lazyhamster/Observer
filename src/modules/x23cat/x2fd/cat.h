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
	wchar_t pszFileName[MAX_PATH];
	io64::file::position offset;
	io64::file::size size;
	
	filebuffer *buffer;
	
	x2catentry() { memset(pszFileName, 0, sizeof(pszFileName)); offset=0; size=0; buffer=0; }
};
//---------------------------------------------------------------------------------
class x2catbuffer : public ext::list<x2catentry *> 
{
	private:
		wchar_t *m_pszFileName;
		wchar_t *m_pszDATName;
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
		
		int error() const { return m_nError; }
		
		bool open(const wchar_t *pszName);
		
		filebuffer * loadFile(x2catentry *entry, int fileType);
};
//---------------------------------------------------------------------------------

#endif // !defined(CAT_INCLUDED)