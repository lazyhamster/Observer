#include "stdafx.h"
#include "IS3CabFile.h"
#include "Utils.h"

namespace IS3
{

IS3CabFile::IS3CabFile()
{
	m_hHeaderFile = INVALID_HANDLE_VALUE;
	memset(&m_Header, 0, sizeof(m_Header));
}

IS3CabFile::~IS3CabFile()
{
	Close();
}

bool IS3CabFile::InternalOpen( HANDLE headerFile )
{
	if (headerFile == INVALID_HANDLE_VALUE)
		return false;

	CABHEADER cabHeader = {0};

	if (!ReadBuffer(headerFile, &cabHeader, sizeof(cabHeader)))
		return false;

	// Validate file

	if (cabHeader.Signature != Z_SIG)
		return false;
	if (cabHeader.cFiles == 0)
		return false;
	if (cabHeader.ArchiveSize != FileSize(headerFile) || cabHeader.offsTOC >= cabHeader.ArchiveSize)
		return false;

	if (!SeekFile(headerFile, cabHeader.offsTOC))
		return false;

	// Read directories
	__int64 filePos = cabHeader.offsTOC;
	for (int i = 0; i < cabHeader.cDirs; i++)
	{
		DIRDESC dir;

		if (!ReadBuffer(headerFile, &dir, sizeof(dir)))
			return false;

		DirEntry de = {0};
		de.NumFiles = dir.cFiles;
		ReadBuffer(headerFile, de.Name, dir.cbNameLength);
		m_vDirs.push_back(de);

		filePos += dir.cbDescSize;
		if (!SeekFile(headerFile, filePos)) return false;
	}

	// Read files from all directories
	for (int i = 0; i < cabHeader.cFiles; i++)
	{
		FILEDESC file;

		if (!ReadBuffer(headerFile, &file, sizeof(file)))
			return false;

		//TODO: Store for future use

		filePos += file.cbDescSize;
		if (!SeekFile(headerFile, filePos)) return false;
	}

	// Remember basic data

	memcpy_s(&m_Header, sizeof(m_Header), &cabHeader, sizeof(cabHeader));
	
	return false;
}

void IS3CabFile::Close()
{
	if (m_hHeaderFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHeaderFile);
	}

	m_vDirs.clear();
	m_vFiles.clear();
}

int IS3CabFile::GetTotalFiles() const
{
	return 0;
}

bool IS3CabFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	return false;
}

int IS3CabFile::ExtractFile( int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx )
{
	return CAB_EXTRACT_READ_ERR;
}

} //namespace IS3
