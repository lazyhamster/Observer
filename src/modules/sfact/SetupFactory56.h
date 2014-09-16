#ifndef SetupFactory56_h__
#define SetupFactory56_h__

#include "SfFile.h"

class SetupFactory56 :
	public SetupFactoryFile
{
private:
	bool CheckSignature(CFileStream* inFile, int64_t offset, int sufVersion);
	int ParseScript(int64_t baseOffset);
	
public:
	SetupFactory56(void);
	~SetupFactory56(void);

	bool Open(CFileStream* inFile);
	int EnumFiles();
};

#endif // SetupFactory56_h__
