#ifndef SetupFactory56_h__
#define SetupFactory56_h__

#include "SfFile.h"

class SetupFactory56 :
	public SetupFactoryFile
{
private:
	int m_nVersion;
	int64_t m_nStartOffset;
	int64_t m_nContentBaseOffset;
	CMemoryStream* m_pScriptData;

	bool CheckSignature(CFileStream* inFile, int64_t offset);
	int ParseScript(int64_t baseOffset);
	
	bool ReadString(AStream* stream, char* buf);
	bool SkipString(AStream* stream);

public:
	SetupFactory56(void);
	~SetupFactory56(void);

	bool Open(CFileStream* inFile);
	void Close();
	int EnumFiles();
	int GetVersion() { return m_nVersion; }
	bool ExtractFile(int index, AStream* outStream);
};

#endif // SetupFactory56_h__
