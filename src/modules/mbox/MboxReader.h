#ifndef MboxReader_h__
#define MboxReader_h__

#include "MailReader.h"

class CMboxReader : public IMailReader
{
public:
	int Scan() override;
	bool CheckSample(const void* sampleBuffer, size_t sampleSize) override;
	const wchar_t* GetFormatName() const override { return L"Unix MBox"; }
};

#endif // MboxReader_h__