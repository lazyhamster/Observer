#ifndef CabSystem_h__
#define CabSystem_h__

#include <Msi.h>

struct openfile_s
{
	MSIHANDLE hMSI;
	const wchar_t* streamName;
};

#endif // CabSystem_h__
