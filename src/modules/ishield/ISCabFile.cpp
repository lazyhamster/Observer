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

int ISCabFile::TransferFile( CFileStream* src, CFileStream* dest, __int64 fileSize, bool decrypt, BYTE* hashBuf, ExtractProcessCallbacks* progress )
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

int ISCabFile::UnpackFile( CFileStream* src, CFileStream* dest, __int64 fileSize, BYTE* hashBuf, ExtractProcessCallbacks* progress )
{
	MD5_CTX md5;
	MD5Init(&md5);
	
	BYTE inputBuffer[EXTRACT_BUFFER_SIZE] = {0};

	size_t outputBufferSize = EXTRACT_BUFFER_SIZE;
	BYTE* outputBuffer = (BYTE*) malloc(outputBufferSize);

	size_t outputDataSize;
	__int64 bytesLeft = fileSize;
	while (bytesLeft > 0)
	{
		WORD blockSize;
		if (!src->ReadBuffer(&blockSize, sizeof(blockSize)) || !blockSize)
			break;

		if (!src->ReadBuffer(inputBuffer, blockSize))
			break;

		if (!UnpackBuffer(inputBuffer, blockSize, outputBuffer, &outputBufferSize, &outputDataSize))
			break;

		bytesLeft -= outputDataSize;
		MD5Update(&md5, outputBuffer, (unsigned int) outputDataSize);

		if (!dest->WriteBuffer(outputBuffer, (DWORD) outputDataSize))
		{
			free(outputBuffer);
			return CAB_EXTRACT_WRITE_ERR;
		}

		if (progress && progress->FileProgress && progress->signalContext)
		{
			if (!progress->FileProgress(progress->signalContext, outputDataSize))
			{
				free(outputBuffer);
				return CAB_EXTRACT_USER_ABORT;
			}
		}
	}

	free(outputBuffer);
	if (hashBuf != NULL)
	{
		MD5Final(hashBuf, &md5);
	}
	return CAB_EXTRACT_OK;
}
