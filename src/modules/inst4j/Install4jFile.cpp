#include "stdafx.h"
#include "Install4jFile.h"

#include "modulecrt/PEHelper.h"

#include "Decomp.h"

constexpr size_t MagicSize = 4;
const uint8_t MAGIC_START[MagicSize] = { 0xD5, 0x13, 0xE4, 0xE8 };
const uint8_t MAGIC_END[MagicSize] = { 0xE8, 0xE4, 0x13, 0xD5 };

static const uint8_t* find_sequence(const uint8_t* data, size_t dataSize, const uint8_t* sequence, size_t sequenceSize)
{
	const uint8_t* searchPtr = data;
	size_t searchSize = dataSize - sequenceSize + 1;
	while (searchPtr = (const uint8_t*) memchr(searchPtr, (int)sequence[0], searchSize))
	{
		if (memcmp(searchPtr, sequence, sequenceSize) == 0)
			return searchPtr;
		
		++searchPtr;
		searchSize = dataSize - (searchPtr - data) - sequenceSize + 1;
		if (searchSize < sequenceSize) break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

CInstall4jFile::CInstall4jFile()
{
	m_pInStream = nullptr;
	m_nArchDataStart = 0;
}

CInstall4jFile::~CInstall4jFile()
{
	Close();
}

bool CInstall4jFile::Open(const wchar_t* path)
{
	Close();
	
	CFileStream* inStream = CFileStream::Open(path, true, false);
	if (!inStream || !inStream->IsValid()) return false;

	if (InternalOpen(inStream))
	{
		m_pInStream = inStream;
		return true;
	}
	
	delete inStream;
	return false;
}

void CInstall4jFile::Close()
{
	if (m_pInStream != nullptr)
	{
		delete m_pInStream;
		m_pInStream = nullptr;
	}
	m_vArchList.clear();
	m_nArchDataStart = 0;
}

bool CInstall4jFile::InternalOpen(CFileStream* inStream)
{
	int64_t nOverlayStart, nOverlaySize;

	if (!FindFileOverlay(inStream, nOverlayStart, nOverlaySize))
		return false;

	inStream->SetPos(nOverlayStart);

	char Sig[MagicSize];
	if (!inStream->ReadBuffer(Sig, sizeof(Sig)))
		return false;
	if (memcmp(Sig, MAGIC_START, sizeof(Sig)) != 0)
		return false;

	// We need to search for starting block end signature and position stream at the end
	// Right now it is unknown how to reach data block without sequence searching
	constexpr size_t bufSize = 32 * 1024;
	auto readBuf = std::make_unique<uint8_t[]>(bufSize);
	
	int64_t bytesLeft = nOverlaySize;
	bool endMagicFound = false;
	while (bytesLeft > MagicSize)
	{
		int64_t streamPos = inStream->GetPos();
		size_t readSize = min((size_t) bytesLeft, bufSize);
		if (!inStream->ReadBuffer(readBuf.get(), readSize))
			return false;

		const uint8_t* magicPos = find_sequence(readBuf.get(), readSize, MAGIC_END, MagicSize);
		if (magicPos)
		{
			size_t offset = magicPos - readBuf.get() + MagicSize;
			inStream->SetPos(streamPos + offset);
			endMagicFound = true;
			break;
		}

		// Signature may not be aligned to 4 byte block so let's step back a little
		inStream->Seek(MagicSize - 1, STREAM_CURRENT);
		bytesLeft -= readSize + MagicSize - 1;
	}

	if (!endMagicFound) return false;

	// For archives list integer values are in big endian

	uint32_t numArchives;
	if (!inStream->Read(&numArchives))
		return false;

	// Just in case we read some wrong value
	numArchives = _byteswap_ulong(numArchives);
	if (numArchives > 100)
		return false;

	char nameBuf[MAX_PATH] = {0};
	uint16_t nameSize;
	uint32_t dummy, arcSize;

	m_vArchList.clear();
	int64_t arcOffset = 0;
	for (uint32_t i = 0; i < numArchives; ++i)
	{
		inStream->Read(&nameSize);
		inStream->ReadBuffer(nameBuf, _byteswap_ushort(nameSize));
		inStream->Read(&dummy);
		inStream->Read(&arcSize);

		DataArchiveInfo info;
		info.Name.assign(nameBuf, _byteswap_ushort(nameSize));
		info.Size = _byteswap_ulong(arcSize);
		info.Offset = arcOffset;

		m_vArchList.push_back(info);
		arcOffset += info.Size;
	}

	m_nArchDataStart = inStream->GetPos();

	// Get compression info
	for (size_t i = 0; i < m_vArchList.size(); ++i)
	{
		DataArchiveInfo& arch = m_vArchList.at(i);
		char sign[2];

		inStream->SetPos(m_nArchDataStart + arch.Offset);
		inStream->ReadBuffer(sign, sizeof(sign));

		arch.IsCompressed = (sign[0] != 'P' || sign[1] != 'K');
	}

	return m_vArchList.size() > 0;
}

bool CInstall4jFile::ExtractArc(size_t index, const wchar_t* destPath)
{
	if (index < m_vArchList.size())
	{
		CFileStream* outStream = CFileStream::Open(destPath, false, true);
		if (!outStream) return false;

		const DataArchiveInfo& arcInfo = m_vArchList.at(index);
		m_pInStream->SetPos(m_nArchDataStart + arcInfo.Offset);
		
		bool ok = false;
		if (arcInfo.IsCompressed)
			ok = LzmaDecomp(m_pInStream, arcInfo.Size, outStream, nullptr);
		else
			ok = outStream->CopyFrom(m_pInStream, arcInfo.Size) == arcInfo.Size;

		delete outStream;
		return ok;
	}
	return false;
}
