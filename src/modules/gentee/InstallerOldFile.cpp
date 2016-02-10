#include "stdafx.h"
#include "InstallerOldFile.h"

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

void InstallerOldFile::GetCompressionName(wchar_t* buf, size_t bufSize)
{
	//
}
