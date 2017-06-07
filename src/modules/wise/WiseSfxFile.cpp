#include "stdafx.h"
#include "WiseSfxFile.h"
#include "modulecrt/PEHelper.h"
#include "zlib/zlib.h"

CWiseSfxFile::CWiseSfxFile()
{
	m_pSource = nullptr;
	m_nStartOffset = -1;
	m_nFileSize = 0;
	m_nCrc32 = 0;
}

CWiseSfxFile::~CWiseSfxFile()
{
	if (m_pSource)
		delete m_pSource;
}

bool CWiseSfxFile::Open( const wchar_t* filePath )
{
	CFileStream* stream = CFileStream::Open(filePath, true, false);
	if (!stream) return false;

	int64_t nSectionStart, nSectionSize;
	if (FindPESection(stream, ".WISE", nSectionStart, nSectionSize))
	{
		stream->SetPos(nSectionStart + 24);

		uint32_t dataSize;
		stream->Read(&dataSize);

		m_nStartOffset = SearchForMSI(stream);
		if ((m_nStartOffset > 0) && (dataSize <= nSectionSize))
		{
			m_nFileSize = dataSize - sizeof(DWORD);

			stream->SetPos(m_nStartOffset + m_nFileSize);
			stream->Read(&m_nCrc32);

			m_pSource = stream;
			return true;
		}
	}
	
	delete stream;
	return false;
}

bool CWiseSfxFile::GetFileInfo( int index, WiseFileRec* infoBuf, bool &noMoreItems )
{
	noMoreItems = (index > 0);
	
	if (index == 0)
	{
		memset(infoBuf, 0, sizeof(WiseFileRec));
		//TODO: set proper name
		wcscpy_s(infoBuf->FileName, sizeof(infoBuf->FileName) / sizeof(infoBuf->FileName[0]), L"installer.msi");
		infoBuf->StartOffset = m_nStartOffset;
		infoBuf->EndOffset = m_nStartOffset + m_nFileSize;
		infoBuf->UnpackedSize = m_nFileSize;
		infoBuf->PackedSize = m_nFileSize;
		infoBuf->CRC32 = m_nCrc32;

		return true;
	}

	return false;
}

#define COPY_SIZE 64 * 1024

bool CWiseSfxFile::ExtractFile( int index, const wchar_t* destPath )
{
	if (index != 0) return false;

	CFileStream* destStream = CFileStream::Open(destPath, false, true);
	if (!destStream) return false;

	m_pSource->SetPos(m_nStartOffset);
	char* copyBuf[COPY_SIZE];
	
	int64_t bytesLeft = m_nFileSize;
	uLong crcVal = crc32(0, NULL, 0);

	while (bytesLeft > 0)
	{
		size_t copySize = (size_t) min(bytesLeft, COPY_SIZE);
		m_pSource->ReadBuffer(copyBuf, copySize);

		crcVal = crc32(crcVal, (Bytef*) copyBuf, (uInt) copySize);
		
		if (!destStream->WriteBuffer(copyBuf, copySize))
		{
			delete destStream;
			return false;
		}

		bytesLeft -= copySize;
	}

	delete destStream;
	return (crcVal == m_nCrc32);
}

int64_t CWiseSfxFile::SearchForMSI( AStream* stream )
{
	int64_t startPos = -1;

	int64_t currPos = stream->GetPos();
	uint8_t buf[1024] = {0};
	if (stream->ReadBuffer(buf, sizeof(buf)))
	{
		for (int i = 0; i < sizeof(buf) - 2; i++)
		{
			if (buf[i] == 0xD0 && buf[i+1] == 0xCF && buf[i+2] == 0x11)
			{
				startPos = currPos + i;
				break;
			}
		}
	}

	return startPos;
}
