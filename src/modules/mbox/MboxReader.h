#ifndef MboxReader_h__
#define MboxReader_h__

#include "MailReader.h"

class CMboxReader : public IMailReader
{
protected:
	virtual bool IsValidFile( FILE* f );

public:
	virtual int Scan();
};

#endif // MboxReader_h__