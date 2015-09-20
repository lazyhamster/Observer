#ifndef BaseGenteeFile_h__
#define BaseGenteeFile_h__

#include "ModuleDef.h"

enum GenteeExtractResult
{
	Success,
	Failure,
	FailedRead,
	FailedWrite,
	PasswordRequired,
	AbortedByUser
};

class BaseGenteeFile
{
protected:
	AStream* m_pStream;

public:
	virtual ~BaseGenteeFile() {}
	
	virtual bool Open(AStream* inStream) = 0;
	virtual void Close() = 0;

	virtual int GetFilesCount() = 0;
	virtual bool GetFileDesc(int index, StorageItemInfo* desc) = 0;

	virtual GenteeExtractResult ExtractFile(int index, AStream* dest, const char* password) = 0;

	virtual void GetFileTypeName(wchar_t* buf, size_t bufSize) = 0;
	virtual void GetCompressionName(wchar_t* buf, size_t bufSize) = 0;
};

#endif // BaseGenteeFile_h__
