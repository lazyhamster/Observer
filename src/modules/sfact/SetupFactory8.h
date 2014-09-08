#ifndef SetupFactory9_h__
#define SetupFactory9_h__

#include "SfFile.h"

class SetupFactory8 :
	public SetupFactoryFile
{
private:
	int64_t m_nStartOffset;
	int64_t m_nContentBaseOffset;
	
	int ParseScript(int64_t baseOffset);
	bool ReadSpecialFile(const wchar_t* fileName);

public:
	SetupFactory8(void);
	~SetupFactory8(void);

	bool Open(CFileStream* inFile);
	void Close();
	int EnumFiles();
	bool ExtractFile(int index, AStream* outStream);
};

#endif // SetupFactory9_h__
