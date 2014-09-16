#ifndef SetupFactory9_h__
#define SetupFactory9_h__

#include "SfFile.h"

class SetupFactory8 :
	public SetupFactoryFile
{
private:
	int ParseScript(int64_t baseOffset);
	bool ReadSpecialFile(const char* fileName);
	bool DetectCompression(EntryCompression &value);
	uint32_t FindFileBlockInScript();

public:
	SetupFactory8(void);
	~SetupFactory8(void);

	bool Open(CFileStream* inFile);
	void Close();
	int EnumFiles();
	bool ExtractFile(int index, AStream* outStream);
};

#endif // SetupFactory9_h__
