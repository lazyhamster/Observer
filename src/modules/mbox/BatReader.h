#ifndef BatReader_h__
#define BatReader_h__

#include "MailReader.h"

class CBatReader : public IMailReader
{
private:
	__int64 m_nDataStartPos;

protected:
	virtual bool IsValidFile( FILE* f );

public:
	virtual int Scan();
	virtual const wchar_t* GetFormatName() const { return L"The Bat! Mail Base"; }
};

#endif // BatReader_h__