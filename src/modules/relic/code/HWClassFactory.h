#ifndef HWClassFactory_h__
#define HWClassFactory_h__

#include "HWAbstractFile.h"

class CHWClassFactory
{
public:
	static CHWAbstractStorage* LoadFile(const wchar_t *FilePath);
};

#endif // HWClassFactory_h__