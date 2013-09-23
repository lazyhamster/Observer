#include "stdafx.h"
#include "IS3CabFile.h"
#include "Utils.h"

extern "C"
{
#include "blast.h"
};

namespace IS3
{

IS3CabFile::IS3CabFile()
{
	m_hHeaderFile = INVALID_HANDLE_VALUE;
	memset(&m_Header, 0, sizeof(m_Header));
	m_pFileList = NULL;
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
	if (cabHeader.ArchiveSize != FileSize(headerFile) || cabHeader.ofsDirList >= cabHeader.ArchiveSize)
		return false;

	if (!SeekFile(headerFile, cabHeader.ofsDirList))
		return false;

	// Read directories
	__int64 filePos = cabHeader.ofsDirList;
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

	if (!SeekFile(headerFile, cabHeader.ofsFileList))
		return false;

	// Read files from all directories
	LPVOID pFileList = malloc(cabHeader.cbFileList);
	if (!ReadBuffer(headerFile, pFileList, cabHeader.cbFileList))
		return false;

	BYTE* listPtr = (BYTE*) pFileList;
	for (int i = 0; i < cabHeader.cFiles; i++)
	{
		// Probably there should be check for invalid entries like in other versions
		FILEDESC* pDesc = (FILEDESC*) listPtr;
		m_vFiles.push_back(pDesc);
		listPtr += pDesc->cbDescSize;
	}

	// Remember basic data

	memcpy_s(&m_Header, sizeof(m_Header), &cabHeader, sizeof(cabHeader));
	m_pFileList = pFileList;
	
	return true;
}

void IS3CabFile::Close()
{
	if (m_hHeaderFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHeaderFile);
	}

	FREE_NULL(m_pFileList);

	m_vDirs.clear();
	m_vFiles.clear();
}

int IS3CabFile::GetTotalFiles() const
{
	return (int) m_vFiles.size();
}

bool IS3CabFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (!m_pFileList || itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return false;

	FILEDESC* pFileDesc = m_vFiles[itemIndex];
	char pFileName[MAX_PATH] = {0};

	char* pDir = NULL; //TODO: get dir name properly
	strncpy_s(pFileName, sizeof(pFileName), (char*) pFileDesc + sizeof(*pFileDesc), pFileDesc->cbNameLength);

	if (!pDir && !pFileName[0])
		return false;

	// Create full path
	char fullName[MAX_PATH] = {0};
	CombinePath(fullName, MAX_PATH, 2, pDir, pFileName);

	// Fill result structure
	MultiByteToWideChar(CP_ACP, 0, fullName, -1, itemInfo->Path, STRBUF_SIZE(itemInfo->Path));
	itemInfo->Size = pFileDesc->cbExpanded;
	itemInfo->Attributes = pFileDesc->Attrs;
	DosDateTimeToFileTime((WORD)pFileDesc->FatDate, (WORD)pFileDesc->FatTime, &itemInfo->ModificationTime);

	return true;
}

struct BufPtr
{
	void* buf;
	size_t bufSize;
};

unsigned int blast_in(void *how, unsigned char **buf)
{
	BufPtr* ptr = (BufPtr*)how;
	*buf = (unsigned char *)ptr->buf;
	return ptr->bufSize;
}

int blast_out(void *how, unsigned char *buf, unsigned len)
{
	HANDLE outFile = (HANDLE) how;
	return WriteBuffer(outFile, buf, len) ? 0 : 1;
}

int IS3CabFile::ExtractFile( int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx )
{
	if (!m_pFileList || itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return CAB_EXTRACT_READ_ERR;

	FILEDESC* pFileDesc = m_vFiles[itemIndex];

	if (!SeekFile(m_hHeaderFile, pFileDesc->ofsCompData))
		return CAB_EXTRACT_READ_ERR;

	BYTE* inBuffer = (BYTE*) malloc(pFileDesc->cbCompressed);
	
	int result = CAB_EXTRACT_READ_ERR;
	if (ReadBuffer(m_hHeaderFile, inBuffer, pFileDesc->cbCompressed))
	{
		BufPtr inhow = {inBuffer, pFileDesc->cbCompressed};
		int blastRet = blast(blast_in, &inhow, blast_out, targetFile);

		switch (blastRet)
		{
			case 0:
				result = CAB_EXTRACT_OK;
				break;
			case 1:
				result = CAB_EXTRACT_WRITE_ERR;
				break;
			default:
				result = CAB_EXTRACT_READ_ERR;
				break;
		}
	}

	free(inBuffer);
		
	return result;
}

} //namespace IS3
