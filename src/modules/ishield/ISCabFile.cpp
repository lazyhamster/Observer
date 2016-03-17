#include "stdafx.h"
#include "ISCabFile.h"
#include "Utils.h"
#include "md5/md5.h"

#include "IS5\IS5CabFile.h"
#include "IS6\IS6CabFile.h"
#include "ISU\ISUCabFile.h"
#include "IS3\IS3CabFile.h"
#include "ISSFX\ISEncSfxFile.h"
#include "ISSFX\ISPlainSfxFile.h"

template <typename T>
ISCabFile* TryOpen(CFileStream* pFile)
{
	T* obj = new T();
	if (obj->Open(pFile))
		return obj;

	delete obj;
	return NULL;
}

#define RNN(type, hf) { auto isf = TryOpen<type>(hf); if (isf) return isf; }

ISCabFile* OpenCab(const wchar_t* filePath)
{
	CFileStream* pFile = CFileStream::Open(filePath, true, false);
	if (pFile == nullptr)
		return NULL;

	RNN(IS5::IS5CabFile, pFile);
	RNN(IS6::IS6CabFile, pFile);
	RNN(ISU::ISUCabFile, pFile);
	RNN(IS3::IS3CabFile, pFile);
	RNN(ISSfx::ISEncSfxFile, pFile);
	RNN(ISSfx::ISPlainSfxFile, pFile);
	
	delete pFile;

	return NULL;
}

void CloseCab(ISCabFile* cab)
{
	if (cab != NULL)
	{
		cab->Close();
		delete cab;
	}
}

bool ISCabFile::Open( CFileStream* headerFile )
{
	headerFile->SetPos(0);
	
	if (InternalOpen(headerFile))
	{
		m_pHeaderFile = headerFile;
		m_sCabPattern = GenerateCabPatern(headerFile->FilePath());
		GenerateInfoFile();
		return true;
	}

	return false;
}

#define EXTRACT_BUFFER_SIZE 64*1024

int ISCabFile::TransferFile( CFileStream* src, AStream* dest, __int64 fileSize, bool decrypt, BYTE* hashBuf, ExtractProcessCallbacks* progress )
{
	MD5_CTX md5;
	MD5Init(&md5);
	
	BYTE copyBuffer[EXTRACT_BUFFER_SIZE];
	DWORD total = 0;
	__int64 bytesLeft = fileSize;
	while (bytesLeft > 0)
	{
		DWORD copySize = (DWORD) min(bytesLeft, sizeof(copyBuffer));

		if (!src->ReadBuffer(copyBuffer, copySize))
			return CAB_EXTRACT_READ_ERR;

		if (decrypt) DecryptBuffer(copyBuffer, copySize, &total);

		if (!dest->WriteBuffer(copyBuffer, copySize))
			return CAB_EXTRACT_WRITE_ERR;

		MD5Update(&md5, copyBuffer, copySize);
		bytesLeft -= copySize;

		if (progress && progress->signalContext && progress->FileProgress)
		{
			if (!progress->FileProgress(progress->signalContext, copySize))
				return CAB_EXTRACT_USER_ABORT;
		}
	}

	MD5Final(hashBuf, &md5);
	return CAB_EXTRACT_OK;
}

int ISCabFile::UnpackFile( std::vector<CFileStream*> &src, std::vector<__int64> &srcOffsets, AStream* dest, __int64 unpackedSize, BYTE* hashBuf, ExtractProcessCallbacks* progress )
{
	if (src.size() == 0 || src.size() != srcOffsets.size())
		return CAB_EXTRACT_READ_ERR;
	
	MD5_CTX md5;
	MD5Init(&md5);
	
	BYTE inputBuffer[EXTRACT_BUFFER_SIZE] = {0};
	int retVal = CAB_EXTRACT_OK;
	size_t srcIndex = 0;
	
	CFileStream* currStream = src[srcIndex];
	if (!currStream->SetPos(srcOffsets[srcIndex]))
		return CAB_EXTRACT_READ_ERR;

	size_t outputBufferSize = EXTRACT_BUFFER_SIZE;
	BYTE* outputBuffer = (BYTE*) malloc(outputBufferSize);
	
	size_t outputDataSize;
	__int64 bytesLeft = unpackedSize;
	while (bytesLeft > 0)
	{
		WORD blockSize;
		if (!currStream->ReadBuffer(&blockSize, sizeof(blockSize)) || !blockSize || (currStream->GetPos() + blockSize) > currStream->GetSize())
		{
			// Try to switch stream to next one
			srcIndex++;
			if (srcIndex < src.size())
			{
				currStream = src[srcIndex];
				if (!currStream->SetPos(srcOffsets[srcIndex]))
				{
					retVal = CAB_EXTRACT_READ_ERR;
					break;
				}
				continue;
			}
			else
			{
				retVal = CAB_EXTRACT_READ_ERR;
				break;
			}
		}

		if (!currStream->ReadBuffer(inputBuffer, blockSize))
		{
			retVal = CAB_EXTRACT_READ_ERR;
			break;
		}

		if (!UnpackBuffer(inputBuffer, blockSize, outputBuffer, &outputBufferSize, &outputDataSize, false))
		{
			retVal = CAB_EXTRACT_READ_ERR;
			break;
		}

		bytesLeft -= outputDataSize;
		MD5Update(&md5, outputBuffer, (unsigned int) outputDataSize);

		if (!dest->WriteBuffer(outputBuffer, (DWORD) outputDataSize))
		{
			retVal = CAB_EXTRACT_WRITE_ERR;
			break;
		}

		if (progress && progress->FileProgress && progress->signalContext)
		{
			if (!progress->FileProgress(progress->signalContext, outputDataSize))
			{
				retVal = CAB_EXTRACT_USER_ABORT;
				break;
			}
		}
	}

	free(outputBuffer);
	if ((hashBuf != NULL) && (retVal == CAB_EXTRACT_OK))
	{
		MD5Final(hashBuf, &md5);
	}
	return retVal;
}

int ISCabFile::UnpackFileOld( CFileStream* src, DWORD packedSize, AStream* dest, DWORD unpackedSize, BYTE* hashBuf, ExtractProcessCallbacks* progress )
{
	static const uint8_t END_OF_CHUNK[4] = { 0x00, 0x00, 0xff, 0xff };
	
	MD5_CTX md5;
	MD5Init(&md5);

	BYTE* pInputBuffer = (BYTE*) malloc(packedSize);
	BYTE* pOutputBuffer = (BYTE*) malloc(unpackedSize);
	int result = CAB_EXTRACT_OK;

	if (!src->ReadBuffer(pInputBuffer, packedSize))
	{
		result = CAB_EXTRACT_READ_ERR;
		goto exit;
	}
	
	size_t bytesLeft = packedSize;
	BYTE* pBufPtr = pInputBuffer;
	size_t outputDataSize;
	size_t outputBufferSize = unpackedSize;
	
	while (bytesLeft > 0)
	{
		BYTE* pMatch = find_bytes(pBufPtr, bytesLeft, END_OF_CHUNK, sizeof(END_OF_CHUNK));
		if (!pMatch)
		{
			result = CAB_EXTRACT_READ_ERR;
			goto exit;
		}

		size_t chunkSize = pMatch - pBufPtr;

		if (!UnpackBuffer(pBufPtr, chunkSize, pOutputBuffer, &outputBufferSize, &outputDataSize, true))
		{
			result = CAB_EXTRACT_READ_ERR;
			goto exit;
		}

		if (!dest->WriteBuffer(pOutputBuffer, (DWORD) outputDataSize))
		{
			result = CAB_EXTRACT_WRITE_ERR;
			goto exit;
		}

		if (progress && progress->FileProgress && progress->signalContext)
		{
			if (!progress->FileProgress(progress->signalContext, outputDataSize))
			{
				result = CAB_EXTRACT_USER_ABORT;
				goto exit;
			}
		}

		MD5Update(&md5, pOutputBuffer, (unsigned int) outputDataSize);

		pBufPtr += chunkSize;
		pBufPtr += sizeof(END_OF_CHUNK);

		bytesLeft -= chunkSize;
		bytesLeft -= sizeof(END_OF_CHUNK);
	}

	if (hashBuf != NULL)
	{
		MD5Final(hashBuf, &md5);
	}

exit:
	free(pInputBuffer);
	free(pOutputBuffer);
	return result;
}
