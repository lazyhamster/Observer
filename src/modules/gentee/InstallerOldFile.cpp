#include "stdafx.h"
#include "InstallerOldFile.h"
#include "modulecrt/PEHelper.h"

#pragma pack(push, 1)

struct gdll_info
{
	uint32_t Offset;
	uint32_t PackedSize;
	uint32_t UnpackedSize;
};

#pragma pack(pop)

void unpack_buf(BYTE* inBuf, uint32_t PackedSize, BYTE* outBuf, uint32_t UnpackedSize)
{
	//
}

//////////////////////////////////////////////////////////////////////////

InstallerOldFile::InstallerOldFile()
{
	//
}

InstallerOldFile::~InstallerOldFile()
{
	//
}

bool InstallerOldFile::Open(AStream* inStream, int64_t startOffset, bool ownStream)
{
	int64_t dataSectionStart, dataSectionSize;
	if (!FindPESection(inStream, ".data", dataSectionStart, dataSectionSize))
		return false;

	char sig[17] = {0};
	if (!inStream->SetPos(dataSectionStart) || !inStream->ReadBuffer(sig, sizeof(sig)))
		return false;
	if (strncmp(sig, "Gentee installer", 16) != 0)
		return false;

	gdll_info dinfo;

	inStream->SetPos(0x3F0);
	inStream->Read(&dinfo);

	inStream->SetPos(dinfo.Offset);

	uint32_t sizeCheck;
	inStream->Read(&sizeCheck);
	if (sizeCheck != dinfo.UnpackedSize)
		return false;

	BYTE* inBuf = (BYTE*) malloc(dinfo.PackedSize);
	BYTE* outBuf = (BYTE*) malloc(dinfo.UnpackedSize);

	inStream->ReadBuffer(inBuf, dinfo.PackedSize);

	//TODO: unpack
	unpack_buf(inBuf, dinfo.PackedSize, outBuf, dinfo.UnpackedSize);

	free(inBuf);
	free(outBuf);

	uint32_t totalUnpackedSize; // for [setuppath] files
	uint32_t u2[4]; // u2[0] = sizeof([setuppath]) + sizeof([temppath])

	inStream->Read(&totalUnpackedSize);
	inStream->ReadBuffer(u2, sizeof(u2));

	//int64_t ovlStart, ovlSize;
	//bool found = FindFileOverlay(inStream, ovlStart, ovlSize);
	
	return false;
}

void InstallerOldFile::Close()
{
	//
}

bool InstallerOldFile::GetFileDesc(int index, StorageItemInfo* desc)
{
	return false;
}

GenteeExtractResult InstallerOldFile::ExtractFile(int index, AStream* dest, const char* password)
{
	return Failure;
}
