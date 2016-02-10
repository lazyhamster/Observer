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
	bool m_fStreamOwned;

	void BaseClose()
	{
		if (m_pStream)
		{
			if (m_fStreamOwned) delete m_pStream;
			m_pStream = nullptr;
		}
	}

public:
	virtual ~BaseGenteeFile() {}
	
	virtual bool Open(AStream* inStream, int64_t startOffset, bool ownStream) = 0;
	virtual void Close() = 0;

	bool Open(AStream* inStream) { return Open(inStream, 0, true); }

	virtual int GetFilesCount() = 0;
	virtual bool GetFileDesc(int index, StorageItemInfo* desc) = 0;

	virtual GenteeExtractResult ExtractFile(int index, AStream* dest, const char* password) = 0;

	virtual const wchar_t* GetFileTypeName() = 0;
	virtual const wchar_t* GetCompressionName() = 0;
};

#endif // BaseGenteeFile_h__
