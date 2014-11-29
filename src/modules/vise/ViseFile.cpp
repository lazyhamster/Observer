#include "stdafx.h"
#include "ViseFile.h"
#include "Unpack.h"

#define VISE_SIGNATURE "ESIV"

#define RFIF(x) if (!(x)) return false;

CViseFile::CViseFile()
{
	m_pInFile = nullptr;
	m_nDataStartOffset = 0;
}

CViseFile::~CViseFile()
{
	Close();
}

void CViseFile::Close()
{
	m_pInFile.reset();
	m_sLocation.clear();
	m_nDataStartOffset = 0;
}

bool CViseFile::Open( const wchar_t* filePath )
{
	CFileStream* inStrPtr = CFileStream::Open(filePath, true, false);
	if (!inStrPtr) return false;

	std::shared_ptr<AStream> inStream(inStrPtr);

	DWORD dataStart;
	char sig[4] = {0};

	// Locate signature
	inStream->Seek(-8, STREAM_END);
	inStream->ReadBuffer(sig, sizeof(sig));

	if (strncmp(sig, VISE_SIGNATURE, 4) != 0)
		return false;

	inStream->Read<DWORD>(&dataStart);

	RFIF(dataStart <= inStream->GetSize());
	RFIF(inStream->Seek(dataStart, STREAM_BEGIN));

	// Second signature check
	inStream->ReadBuffer(sig, sizeof(sig));
	if (strncmp(sig, VISE_SIGNATURE, 4) != 0)
		return false;

	// Little bit of black magic
	inStream->Seek(12, STREAM_CURRENT);
	BYTE skipSize;
	while (inStream->Read<BYTE>(&skipSize) && skipSize != 0)
	{
		skipSize &= 0x7f;
		inStream->Seek(skipSize, STREAM_CURRENT);
	}

	// Read files

	RFIF(ReadServiceFiles(inStream));
	RFIF(ReadViseData1(inStream));
	RFIF(ReadServiceFiles(inStream));
	RFIF(ReadViseScript(inStream));

	//TODO: continue

	// Store file params
	m_pInFile = inStream;
	m_sLocation = filePath;
	m_nDataStartOffset = dataStart;

	return true;
}

const ViseFileEntry& CViseFile::GetFileInfo( size_t index )
{
	ViseFileEntry& entry = m_vFiles.at(index);
	if (entry.PackedSize > 0 && entry.UnpackedSize == 0)
	{
		uint32_t unpackedSize = 0;
		m_pInFile->SetPos(entry.StartOffset);
		if (zUnpackData(m_pInFile.get(), entry.PackedSize, nullptr, &unpackedSize, nullptr))
		{
			entry.UnpackedSize = unpackedSize;
		}
	}
	return entry;
}

bool CViseFile::ReadServiceFiles( std::shared_ptr<AStream> inStream )
{
	int16_t fileCount = 0;
	int32_t unknown;
	int32_t compressedSize;
	BYTE fileNameSize;
	char fileNameBuf[MAX_PATH];

	RFIF(inStream->Read(&fileCount) && fileCount > 0);
	
	for (int i = 0; i < fileCount; i++)
	{
		ViseFileEntry entry;
		
		RFIF(inStream->Read(&fileNameSize) && fileNameSize > 0);
		RFIF(inStream->ReadBuffer(fileNameBuf, fileNameSize));

		fileNameBuf[fileNameSize] = '\0';

		RFIF(inStream->Read(&unknown));
		RFIF(inStream->Read(&unknown));
		RFIF(inStream->Read(&unknown));

		RFIF(inStream->Read(&compressedSize));

		entry.Name = fileNameBuf;
		entry.PackedSize = compressedSize;
		entry.UnpackedSize = 0;
		entry.StartOffset = inStream->GetPos();

		inStream->Seek(compressedSize, STREAM_CURRENT);

		m_vFiles.push_back(entry);
	} // for

	return true;
}

bool CViseFile::ReadViseData1( std::shared_ptr<AStream> inStream )
{
	char strTmpBuf[256];
	uint32_t serviceDataPackedSize;
	
	inStream->Seek(4, STREAM_CURRENT);

	for (int i = 0; i < 2; i++)
	{
		RFIF(ReadViseString(inStream, strTmpBuf));
	}

	inStream->Seek(2, STREAM_CURRENT);

	for (int i = 0; i < 2; i++)
	{
		RFIF(ReadViseString(inStream, strTmpBuf));
	}

	inStream->Seek(2, STREAM_CURRENT);
	inStream->Read(&serviceDataPackedSize);

	inStream->Seek(serviceDataPackedSize, STREAM_CURRENT);
	
	int32_t numValues;

	inStream->Seek(1, STREAM_CURRENT);
	inStream->Read(&numValues);

	for (int j = 0; j < numValues; j++)
	{
		inStream->Seek(103, STREAM_CURRENT);
		RFIF(ReadViseString(inStream, strTmpBuf));
		inStream->Seek(14, STREAM_CURRENT);
	}

	inStream->Read(&numValues);
	for (int j = 0; j < numValues; j++)
	{
		RFIF(ReadViseString(inStream, strTmpBuf));
		RFIF(ReadViseString(inStream, strTmpBuf));
	}

	inStream->Read(&numValues);
	for (int j = 0; j < numValues; j++)
	{
		inStream->Seek(16, STREAM_CURRENT);

		RFIF(ReadViseString(inStream, strTmpBuf));
		RFIF(ReadViseString(inStream, strTmpBuf));
	}

	inStream->Read(&numValues);
	for (int j = 0; j < numValues; j++)
	{
		inStream->Seek(16, STREAM_CURRENT);

		RFIF(ReadViseString(inStream, strTmpBuf));
		RFIF(ReadViseString(inStream, strTmpBuf));
		RFIF(ReadViseString(inStream, strTmpBuf));
		RFIF(ReadViseString(inStream, strTmpBuf));
		RFIF(ReadViseString(inStream, strTmpBuf));

		inStream->Seek(2, STREAM_CURRENT);
	}

	return true;
}

bool CViseFile::ReadViseString( std::shared_ptr<AStream> inStream, char* strBuf )
{
	int16_t strSize;
	RFIF(inStream->Read(&strSize) && (strSize >= 0));
	if (strSize > 0)
	{
		RFIF(inStream->ReadBuffer(strBuf, strSize));
		strBuf[strSize] = '\0';
	}

	return true;
}

bool CViseFile::ReadViseScript( std::shared_ptr<AStream> inStream )
{
	return true;
}

bool CViseFile::ExtractFile( size_t fileIndex, AStream* dest )
{
	return false;
}
