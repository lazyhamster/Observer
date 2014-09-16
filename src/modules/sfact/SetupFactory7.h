#ifndef SetupFactory7_h__
#define SetupFactory7_h__

#include "SfFile.h"

class SetupFactory7 :
	public SetupFactoryFile
{
private:
public:
	SetupFactory7(void);
	~SetupFactory7(void);

	bool Open(CFileStream* inFile);
	int EnumFiles();
};

#endif // SetupFactory7_h__
