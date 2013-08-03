#include "stdafx.h"
#include "ISCabFile.h"
#include "Utils.h"
#include "md5/md5.h"

#include "IS5\IS5CabFile.h"
#include "IS6\IS6CabFile.h"
#include "ISU\ISUCabFile.h"

ISCabFile* OpenCab(const wchar_t* filePath)
{
	HANDLE hFile = OpenFileForRead(filePath);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	
	IS5::IS5CabFile* is5 = new IS5::IS5CabFile();
	if (is5->Open(hFile, filePath))
	{
		return is5;
	}
	delete is5;

	IS6::IS6CabFile* is6 = new IS6::IS6CabFile();
	if (is6->Open(hFile, filePath))
	{
		return is6;
	}
	delete is6;

	ISU::ISUCabFile* isu = new ISU::ISUCabFile();
	if (isu->Open(hFile, filePath))
	{
		return isu;
	}
	delete isu;

	CloseHandle(hFile);
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

bool ISCabFile::Open( HANDLE headerFile, const wchar_t* heaerFilePath )
{
	SetFilePointer(headerFile, 0, NULL, FILE_BEGIN);
	
	if (InternalOpen(headerFile))
	{
		m_hHeaderFile = headerFile;
		m_sCabPattern = GenerateCabPatern(heaerFilePath);
		GenerateInfoFile();
		return true;
	}

	return false;
}

#define EXTRACT_BUFFER_SIZE 64*1024

int ISCabFile::TransferFile( HANDLE src, HANDLE dest, __int64 fileSize, bool decrypt, BYTE* hashBuf, ExtractProcessCallbacks* progress )
{
	MD5_CTX md5;
	MD5Init(&md5);
	
	BYTE copyBuffer[EXTRACT_BUFFER_SIZE];
	DWORD total = 0;
	__int64 bytesLeft = fileSize;
	while (bytesLeft > 0)
	{
		DWORD copySize = (DWORD) min(bytesLeft, sizeof(copyBuffer));

		if (!ReadBuffer(src, copyBuffer, copySize))
			return CAB_EXTRACT_READ_ERR;

		if (decrypt) DecryptBuffer(copyBuffer, copySize, &total);

		if (!WriteBuffer(dest, copyBuffer, copySize))
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

int ISCabFile::UnpackFile( HANDLE src, HANDLE dest, __int64 fileSize, BYTE* hashBuf, ExtractProcessCallbacks* progress )
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
		if (!ReadBuffer(src, &blockSize, sizeof(blockSize)) || !blockSize)
			break;

		if (!ReadBuffer(src, inputBuffer, blockSize))
			break;

		if (!UnpackBuffer(inputBuffer, blockSize, outputBuffer, &outputBufferSize, &outputDataSize))
			break;

		bytesLeft -= outputDataSize;
		MD5Update(&md5, outputBuffer, outputDataSize);

		if (!WriteBuffer(dest, outputBuffer, outputDataSize))
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
	MD5Final(hashBuf, &md5);
	return CAB_EXTRACT_OK;
}
