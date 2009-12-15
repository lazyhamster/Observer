#ifndef NsisBzipDecoder_h__
#define NsisBzipDecoder_h__

#include "Common/MyCom.h"

#include "7zip/ICoder.h"
#include "7zip/Common/InBuffer.h"

#define IBUFSIZE 16384
#define OBUFSIZE 32768

namespace NCompress {
namespace NBzip2 {
namespace NDecoder {

#include "nbzip2/bzlib.h"

class CDecoder:
	public ICompressSetInStream,
	public ICompressSetOutStreamSize,
	public ISequentialInStream,
	public CMyUnknownImp
{
private:
	CInBuffer m_InBuffer;
	DState* m_bzState;
	char m_pBufIn[IBUFSIZE];
	char m_pBufOut[OBUFSIZE];
	int m_nBufInAvail;
	int m_nBufOutAvail;
	bool m_fStreamEnd;

public:
	CDecoder();
	~CDecoder();

	MY_UNKNOWN_IMP4(
		ICompressGetInStreamProcessedSize,
		ICompressSetInStream,
		ICompressSetOutStreamSize,
		ISequentialInStream
	)

	// ICompressSetInStream
	STDMETHOD(SetInStream)(ISequentialInStream *inStream);
	STDMETHOD(ReleaseInStream)();

	// ICompressSetOutStreamSize
	STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);

	// ISequentialInStream
	STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);

	// IGetInStreamProcessedSize
	STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);
};

}}}

#endif // NsisBzipDecoder_h__