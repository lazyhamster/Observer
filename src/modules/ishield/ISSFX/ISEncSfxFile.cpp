#include "stdafx.h"
#include "ISEncSfxFile.h"
#include "Utils.h"
#include "PEHelper.h"
#include "zlib\zlib.h"

ISSfx::ISEncSfxFile::ISEncSfxFile()
{
	m_hHeaderFile = INVALID_HANDLE_VALUE;
}

ISSfx::ISEncSfxFile::~ISEncSfxFile()
{
	Close();
}

bool ISSfx::ISEncSfxFile::InternalOpen( HANDLE headerFile )
{
	__int64 nOverlayStart, nOverlaySize;

	if (!FindFileOverlay(headerFile, nOverlayStart, nOverlaySize))
		return false;

	SeekFile(headerFile, nOverlayStart);

	char Sig[14];
	if (!ReadBuffer(headerFile, Sig, sizeof(Sig)) || strcmp(Sig, "InstallShield"))
		return false;

	DWORD NumFiles;
	if (!ReadBuffer(headerFile, &NumFiles, sizeof(NumFiles)) || NumFiles == 0)
		return false;

	if (!SeekFile(headerFile, FilePos(headerFile) + 0x1C))
		return false;

	for (DWORD i = 0; i < NumFiles; i++)
	{
		SfxFileEntry fe;
		ReadBuffer(headerFile, &fe.Header, sizeof(fe.Header));
		fe.StartOffset = FilePos(headerFile);
		m_vFiles.push_back(fe);

		if ((fe.StartOffset + fe.Header.CompressedSize > nOverlayStart + nOverlaySize) || !SeekFile(headerFile, fe.StartOffset + fe.Header.CompressedSize))
			return false;
	}

	m_hHeaderFile = headerFile;
	
	return m_vFiles.size() > 0;
}

void ISSfx::ISEncSfxFile::Close()
{
	if (m_hHeaderFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHeaderFile);
	}

	m_vFiles.clear();
}

int ISSfx::ISEncSfxFile::GetTotalFiles() const
{
	return (int) m_vFiles.size();
}

bool ISSfx::ISEncSfxFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return false;

	const SfxFileEntry &fileEntry = m_vFiles[itemIndex];

	__int64 unpackedSize = 0;
	switch (fileEntry.Header.Type)
	{
		case 0:
			unpackedSize = fileEntry.Header.CompressedSize;
			break;
		case 2:
			DecodeFile(&fileEntry, NULL, 0, &unpackedSize, NULL);
			break;
		case 6:
			DecodeFile(&fileEntry, NULL, 1024, &unpackedSize, NULL);
			break;
	}

	// Fill result structure
	MultiByteToWideChar(CP_ACP, 0, fileEntry.Header.Name, -1, itemInfo->Path, STRBUF_SIZE(itemInfo->Path));
	itemInfo->Size = unpackedSize;
		
	return true;
}

int ISSfx::ISEncSfxFile::ExtractFile( int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx )
{
	if (itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return CAB_EXTRACT_READ_ERR;

	const SfxFileEntry &fileEntry = m_vFiles[itemIndex];
	__int64 unpackedSize = 0;

	switch (fileEntry.Header.Type)
	{
		case 0:
			return CopyPlainFile(&fileEntry, targetFile, &unpackedSize, &progressCtx);
		case 2:
			return DecodeFile(&fileEntry, targetFile, 0, &unpackedSize, &progressCtx);
		case 6:
			return DecodeFile(&fileEntry, targetFile, 1024, &unpackedSize, &progressCtx);
	}
	
	return CAB_EXTRACT_READ_ERR;
}

static std::string PrepareKey(const char* fileName)
{
	const char* SpecialKey = "\xEC\xCA\x79\xF8";

	size_t keyLen = strlen(fileName);
	size_t specLen = strlen(SpecialKey);

	std::string retVal(keyLen, ' ');

	size_t specIndex = 0;
	for (size_t i = 0; i < keyLen; i++)
	{
		retVal[i] = fileName[i] ^ SpecialKey[specIndex];
		specIndex = (specIndex + 1 < specLen) ? specIndex + 1 : 0;
	}

	return retVal;
}

static inline BYTE SwapBits(BYTE input)
{
	return (input >> 4) | (input << 4);
}

#define EXTRACT_BUFFER_SIZE 32*1024

int ISSfx::ISEncSfxFile::DecodeFile(const SfxFileEntry *pEntry, HANDLE dest, DWORD chunkSize, __int64 *unpackedSize, ExtractProcessCallbacks *pctx) const
{
	*unpackedSize = 0;

	std::string strDecryptKey = PrepareKey(pEntry->Header.Name);
	size_t decryptKeySize = strDecryptKey.size();
	const char* decryptKey = strDecryptKey.c_str();

	SeekFile(m_hHeaderFile, pEntry->StartOffset);

	size_t offsChunk = 0, offsKey = 0;
	__int64 bytesLeft = pEntry->Header.CompressedSize;
	BYTE readBuffer[EXTRACT_BUFFER_SIZE] = {0};
	DWORD readDataSize;

	z_stream strm = {0};
	if (pEntry->Header.IsCompressed && (inflateInit2(&strm, MAX_WBITS) != Z_OK))
		return CAB_EXTRACT_READ_ERR;

	size_t unpackBufferSize = EXTRACT_BUFFER_SIZE * 2;
	BYTE* unpackBuffer = (BYTE*) malloc(unpackBufferSize);

	int result = CAB_EXTRACT_OK;
	while (bytesLeft > 0)
	{
		readDataSize = (DWORD) min(bytesLeft, EXTRACT_BUFFER_SIZE);
		
		if (!ReadBuffer(m_hHeaderFile, readBuffer, readDataSize))
		{
			result = CAB_EXTRACT_READ_ERR;
			break;
		}

		// Decrypting buffer
		for (DWORD i = 0; i < readDataSize; i++)
		{
			readBuffer[i] = SwapBits(readBuffer[i]) ^ (BYTE) decryptKey[offsKey];

			offsKey++;
			offsChunk++;

			if (offsKey >= decryptKeySize)
			{
				offsKey = 0;
			}
			if (offsChunk == chunkSize)
			{
				offsChunk = 0;
				offsKey = 0;
			}
		}

		__int64 nWriteSize = 0;

		// Decompress if needed
		if (pEntry->Header.IsCompressed)
		{
			strm.avail_in = readDataSize;
			strm.next_in = readBuffer;

			do 
			{
				strm.avail_out = (uInt) unpackBufferSize;
				strm.next_out = unpackBuffer;
				
				int ret = inflate(&strm, Z_NO_FLUSH);
				if (ret == Z_NEED_DICT)
					ret = Z_DATA_ERROR;
				if (ret < 0) break;

				__int64 haveBytes = unpackBufferSize - strm.avail_out;

				if (dest && !WriteBuffer(dest, unpackBuffer, (DWORD) haveBytes))
				{
					result = CAB_EXTRACT_WRITE_ERR;
					break;
				}

				nWriteSize += haveBytes;

			} while (strm.avail_out == 0);

			if (result != CAB_EXTRACT_OK) break;
		}
		else
		{
			nWriteSize = readDataSize;
			if (dest && !WriteBuffer(dest, readBuffer, readDataSize))
			{
				result = CAB_EXTRACT_WRITE_ERR;
				break;
			}
		}

		if (pctx && pctx->FileProgress)
		{
			if (!pctx->FileProgress(pctx->signalContext, nWriteSize))
			{
				result = CAB_EXTRACT_USER_ABORT;
				break;
			}
		}

		bytesLeft -= readDataSize;
		*unpackedSize += nWriteSize;
	}

	free(unpackBuffer);
	if (pEntry->Header.IsCompressed) inflateEnd(&strm);

	return result;
}

int ISSfx::ISEncSfxFile::CopyPlainFile( const SfxFileEntry *pEntry, HANDLE dest, __int64 *unpackedSize, ExtractProcessCallbacks *pctx ) const
{
	// File can not be compressed
	if (pEntry->Header.Type == 0 && pEntry->Header.IsCompressed != 0)
		return CAB_EXTRACT_READ_ERR;
	
	*unpackedSize = 0;
	SeekFile(m_hHeaderFile, pEntry->StartOffset);

	__int64 bytesLeft = pEntry->Header.CompressedSize;
	BYTE readBuffer[EXTRACT_BUFFER_SIZE] = {0};
	DWORD readDataSize;

	int result = CAB_EXTRACT_OK;
	while (bytesLeft > 0)
	{
		readDataSize = (DWORD) min(bytesLeft, EXTRACT_BUFFER_SIZE);

		if (!ReadBuffer(m_hHeaderFile, readBuffer, readDataSize))
		{
			result = CAB_EXTRACT_READ_ERR;
			break;
		}

		if (dest && !WriteBuffer(dest, readBuffer, readDataSize))
		{
			result = CAB_EXTRACT_WRITE_ERR;
			break;
		}

		if (pctx && pctx->FileProgress)
		{
			if (!pctx->FileProgress(pctx->signalContext, readDataSize))
			{
				result = CAB_EXTRACT_USER_ABORT;
				break;
			}
		}

		bytesLeft -= readDataSize;
		*unpackedSize += readDataSize;
	}

	return result;
}
