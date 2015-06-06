#include "stdafx.h"
#include "ViseFile.h"
#include "Unpack.h"

#define VISE_SIGNATURE "ESIV"

#define RFIF(x) if (!(x)) return false;

CViseFile::CViseFile()
{
	Close();
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
	m_nScriptStartOffset = 0;
	m_vFiles.clear();
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
	uint8_t fileNameSize;
	char fileNameBuf[MAX_PATH];
	WORD wFatDate, wFatTime;
	FILETIME localTime, lastWriteTime;

	RFIF(inStream->Read(&fileCount) && fileCount > 0);
	
	for (int i = 0; i < fileCount; i++)
	{
		ViseFileEntry entry;
		
		RFIF(inStream->Read(&fileNameSize) && fileNameSize > 0);
		RFIF(inStream->ReadBuffer(fileNameBuf, fileNameSize));

		fileNameBuf[fileNameSize] = '\0';

		RFIF(inStream->Read(&wFatDate));
		RFIF(inStream->Read(&wFatTime));
		RFIF(inStream->Read(&unknown));
		RFIF(inStream->Read(&unknown));

		RFIF(inStream->Read(&compressedSize));

		DosDateTimeToFileTime(wFatDate, wFatTime, &localTime);
		LocalFileTimeToFileTime(&localTime, &lastWriteTime);

		entry.Name = fileNameBuf;
		entry.PackedSize = compressedSize;
		entry.UnpackedSize = 0;
		entry.LastWriteTime = lastWriteTime;
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
		// This value is not stable
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
	}
	strBuf[strSize] = '\0';

	return true;
}

bool CViseFile::ReadViseScript( std::shared_ptr<AStream> inStream )
{
	uint16_t setupTypeCount;
	char strSetupType[256];
	char strTmp[1024];

	m_nScriptStartOffset = inStream->GetPos();

	RFIF(inStream->Read(&setupTypeCount));
	for (uint16_t i = 0; i < setupTypeCount; i++)
	{
		uint32_t typeIndex;
		uint16_t num16;
		
		RFIF(ReadViseString(inStream, strSetupType));
		RFIF(inStream->Skip(2));
		RFIF(ReadViseString(inStream, strTmp));
		RFIF(inStream->Skip(3)); // 2 + 1
		RFIF(ReadViseString(inStream, strTmp));

		RFIF(inStream->Skip(10));
		RFIF(inStream->Skip(16));
		
		RFIF(inStream->Skip(4));
		RFIF(inStream->Read(&typeIndex));
		RFIF(inStream->Skip(36)); // Filled with FF, blocks by 4
		RFIF(inStream->Skip(1));
		RFIF(inStream->Read(&num16));
		RFIF(inStream->Skip(num16 * 5));
		RFIF(inStream->Skip(2));
		RFIF(ReadViseString(inStream, strTmp));
		// Magic numbers are not stable
	}

	uint32_t objectCount;
	uint32_t objectType;
	
	char filename[MAX_PATH];
	char directory[MAX_PATH];
	uint32_t originalSize, packedSize, offset;

	RFIF(inStream->Read(&objectCount));
	for (uint32_t i = 0; i < objectCount; i++)
	{
		RFIF(inStream->Read(&objectType));

		switch (objectType)
		{
			case 0x02:
				{
					RFIF(inStream->Seek(0x65, STREAM_CURRENT));
					RFIF(ReadViseString(inStream, filename));
					RFIF(inStream->Seek(4, STREAM_CURRENT));  // cam be FatTime and FatDate, 2 bytes each
					RFIF(inStream->Read(&originalSize));
					RFIF(inStream->Read(&packedSize));
					RFIF(inStream->Seek(4, STREAM_CURRENT));
					RFIF(inStream->Read(&offset));
					RFIF(inStream->Seek(0x25, STREAM_CURRENT));
					RFIF(ReadViseString(inStream, directory));
					RFIF(inStream->Seek(0x28, STREAM_CURRENT));

					ViseFileEntry fe;
					fe.Name = directory;
					fe.PackedSize = packedSize;
					fe.UnpackedSize = originalSize;
					fe.StartOffset = offset;

					m_vFiles.push_back(fe);
				}
				break;
			case 0x05:
				{
					int8_t unknown;

					RFIF(inStream->Seek(0x66, STREAM_CURRENT));
					RFIF(ReadViseString(inStream, strTmp));

					RFIF(inStream->Read(&unknown));
					if (2 != unknown)
					{
						return false;
					}

					for (int j = 0; j < 3; j++)
					{
						RFIF(ReadViseString(inStream, strTmp));
					}

					RFIF(inStream->Seek(0x5, STREAM_CURRENT));
				}
				break;
			case 0x10:
				{
					int8_t unknown;

					RFIF(inStream->Skip(0x65));
										
					RFIF(ReadViseString(inStream, strTmp)); // key
					RFIF(inStream->Read(&unknown));
					if (unknown)
					{
						return false;
					}
					RFIF(ReadViseString(inStream, strTmp)); // value
					RFIF(inStream->Skip(1));
					RFIF(ReadViseString(inStream, strTmp));
					RFIF(ReadViseString(inStream, strTmp));
					RFIF(ReadViseString(inStream, strTmp));
				}
				break;
			case 0x11:
				{
					RFIF(inStream->Seek(0x65, STREAM_CURRENT));
					for (int j = 0; j < 2; j++)
					{
						RFIF(ReadViseString(inStream, strTmp));
					}
					RFIF(inStream->Seek(0x5, STREAM_CURRENT));
				}
				break;
			case 0x1f:
				RFIF(inStream->Seek(0x65, STREAM_CURRENT));
				break;
			case 0x24:
				{
					int string_count = -1;
					int8_t values[6];

					RFIF(inStream->Seek(0x65, STREAM_CURRENT));
					RFIF(inStream->ReadBuffer(values, sizeof(values)));

					switch (values[0])
					{
						case 1+2:
							switch (values[4])
							{
							case 0:
								string_count = 5;
								break;
							case 1:
								string_count = 1;
								break;
							default:
								return false;
							}
							break;

						case 2+4:
							string_count = 3;
							break;

						case 1+8:
							string_count = 6;
							break;

						case 1+4+8:
							string_count = 1;
							break;

						case 16:
							/* this is not correct */
							string_count = 1;
							break;

						default:
							return false;
					}

					for (int j = 0; j < string_count; j++)
					{
						RFIF(ReadViseString(inStream, strTmp));
					}
				}
				break;
			case 0x25:
				RFIF(inStream->Seek(0x67, STREAM_CURRENT));
				break;
			default:
				// Unknown object type
				return false;
		}
	}
	
	return true;
}

bool CViseFile::ExtractFile( size_t fileIndex, AStream* dest )
{
	ViseFileEntry &entry = m_vFiles[fileIndex];
	uint32_t crc;

	m_pInFile->SetPos(entry.StartOffset);
	if (zUnpackData(m_pInFile.get(), entry.PackedSize, dest, nullptr, &crc))
	{
		return true;
	}

	return false;
}
