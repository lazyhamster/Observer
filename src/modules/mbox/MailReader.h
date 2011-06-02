#ifndef MailReader_h__
#define MailReader_h__

#include "ModuleDef.h"

struct MBoxItem
{
	__int64 StartPos;
	__int64 EndPos;

	std::wstring Subject;
	std::wstring Sender;

	time_t Date;
	int TimeZone;

	__int64 Size() const { return EndPos - StartPos; }
};

class IMailReader
{
protected:
	FILE* m_pSrcFile;
	std::vector<MBoxItem> m_vItems;

	virtual bool IsValidFile(FILE* f) = 0;

public:
	IMailReader() : m_pSrcFile(NULL) {}
	virtual ~IMailReader() { Close(); }

	bool Open(const wchar_t* filepath);
	void Close();

	virtual int Scan() = 0;

	int GetItemsCount() { return (int) m_vItems.size(); }
	const MBoxItem& GetItem(int index) { return m_vItems[index]; }

	int Extract(int itemindex, const wchar_t* destpath);
};

std::wstring ConvertString(const char* src);

#endif // MailReader_h__