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
	return S_OK;
}

STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
	if (processedSize)
		*processedSize = 0;

	UInt32 nRead = m_InBuffer.ReadBytes((Byte *) &m_pBufIn[0], size);

	m_bzState->next_in = m_pBufIn;
	m_bzState->avail_in = nRead;
	m_bzState->next_out = m_pBufOut;
	m_bzState->avail_out = OBUFSIZE;

	int dec_res = BZ2_bzDecompress(m_bzState);
	if (dec_res >= 0)
	{
		size_t nDataSize = m_bzState->next_out - m_pBufOut;
		memcpy(data, m_pBufOut, nDataSize);
		if (processedSize)
			*processedSize = nDataSize;
		return S_OK;
	}

	return S_FALSE;
}

}}}