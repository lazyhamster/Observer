#ifndef MboxReader_h__
#define MboxReader_h__

#include "MboxItems.h"

class CMboxReader
{
private:
	FILE* m_pFile;

public:
	std::vector<MBoxItem> vItems;
	
	CMboxReader() : m_pFile(NULL) {}
	~CMboxReader() { Close(); }

	bool Open(const wchar_t* filepath);
	void Close();

	bool Extract(int itemindex, const wchar_t* destpath);
};

#endif // MboxReader_h__