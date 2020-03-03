#ifndef MailReader_h__
#define MailReader_h__

#include "ModuleDef.h"

struct MBoxItem
{
	__int64 StartPos;
	__int64 EndPos;

	std::wstring Subject;
	std::wstring Sender;
	bool IsDeleted;

	time_t DateUtc;

	MBoxItem() : StartPos(0), EndPos(0), IsDeleted(false),
		DateUtc(0), Subject(L""), Sender(L"")
	{}

	__int64 GetSize() const { return EndPos - StartPos; }
};

class IMailReader
{
protected:
	FILE* m_pSrcFile;
	std::vector<MBoxItem> m_vItems;

	const char* GetSenderAddress(GMimeMessage* message);

public:
	IMailReader() : m_pSrcFile(NULL) {}
	virtual ~IMailReader() { Close(); }

	bool Open(const wchar_t* filepath);
	void Close();

	virtual int Scan() = 0;
	virtual bool CheckSample(const void* sampleBuffer, size_t sampleSize) = 0;
	virtual const wchar_t* GetFormatName() const { return L"Mail Container"; }

	int GetItemsCount() const { return (int) m_vItems.size(); }
	const MBoxItem& GetItem(int index) { return m_vItems[index]; }

	int Extract(int itemindex, const wchar_t* destpath);
};

std::wstring ConvertString(const char* src);
void SanitizeString(std::wstring &str);
time_t ConvertGDateTimeToUnixUtc(GDateTime* gdt);

#endif // MailReader_h__