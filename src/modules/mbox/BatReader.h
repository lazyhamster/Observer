#ifndef BatReader_h__
#define BatReader_h__

#include "MailReader.h"

class CBatReader : public IMailReader
{
public:
	int Scan() override;
	bool CheckSample(const void* sampleBuffer, size_t sampleSize) override;
	const wchar_t* GetFormatName() const override { return L"The Bat! Mail Base"; }
};

#endif // BatReader_h__