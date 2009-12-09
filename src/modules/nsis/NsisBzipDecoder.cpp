#include "StdAfx.h"
#include "NsisBzipDecoder.h"

namespace NCompress {
namespace NBzip2 {
namespace NDecoder {

CDecoder::CDecoder()
{
	m_InBuffer.Create(IBUFSIZE);
	m_InBuffer.Init();
	
	m_bzState = new DState;
	memset(m_bzState, 0, sizeof(DState));
}

CDecoder::~CDecoder()
{
	delete m_bzState;
}

STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream)
{
	m_InBuffer.SetStream(inStream);
	return S_OK;
}

STDMETHODIMP CDecoder::ReleaseInStream()
{
	m_InBuffer.ReleaseStream();
	return S_OK;
}

STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value)
{
	if (value == NULL)
		return E_INVALIDARG;
	*value = m_InBuffer.GetProcessedSize();
	return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 * outSize)
{
	BZ2_bzDecompressInit(m_bzState);
	m_InBuffer.Init();
	return S_OK;
}

STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
	if (processedSize)
		*processedSize = 0;

	UInt32 nBytesLeft = size;
	UInt32 nProcessedCount = 0;
	char* outbuf = (char*) data;

	while (nBytesLeft > 0)
	{
		UInt32 nRead = m_InBuffer.ReadBytes((Byte *) &m_pBufIn[0], (nBytesLeft < IBUFSIZE) ? nBytesLeft : IBUFSIZE);

		m_bzState->next_in = m_pBufIn;
		m_bzState->avail_in = nRead;

		int dec_res;
		do 
		{
			m_bzState->next_out = m_pBufOut;
			m_bzState->avail_out = OBUFSIZE;

			dec_res = BZ2_bzDecompress(m_bzState);
			if (dec_res < 0) return S_FALSE;

			UInt32 nDataSize = m_bzState->next_out - m_pBufOut;
			
			// if there's no output, more input is needed
			if (nDataSize == 0)	break;

			memcpy(outbuf, m_pBufOut, nDataSize);
			outbuf += nDataSize;

			nProcessedCount += nDataSize;
		} while (m_bzState->avail_in && (dec_res != BZ_STREAM_END));

		nBytesLeft -= nRead;
		if (dec_res == BZ_STREAM_END) break;
	}

	if (processedSize)
		*processedSize = nProcessedCount;
	
	return S_OK;
}

}}}