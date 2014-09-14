#include "stdafx.h"
#include "ISPlainSfxFile.h"
#include "Utils.h"
#include "modulecrt/PEHelper.h"

ISSfx::ISPlainSfxFile::ISPlainSfxFile()
{
	m_pHeaderFile = nullptr;
}

ISSfx::ISPlainSfxFile::~ISPlainSfxFile()
{
	Close();
}

bool ISSfx::ISPlainSfxFile::InternalOpen( CFileStream* headerFile )
{
	// Since this type of storage does not have signature we will rely on manifest
	std::string strManifest = GetManifest(headerFile->FilePath());
	if (strManifest.find("InstallShield.Setup") == std::string::npos)
		return false;

	__int64 nOverlayStart, nOverlaySize;

	if (!FindFileOverlay(headerFile, nOverlayStart, nOverlaySize) || nOverlaySize < 2048)  // 2048 is a random number
		return false;

	headerFile->SetPos(nOverlayStart);

	// Unicode version has number of files at the start
	// So if first 4 bytes produce reasonable number then let's assume it is unicode
	DWORD numFiles;
	headerFile->ReadBuffer(&numFiles, sizeof(numFiles));
	bool fIsUnicode = (numFiles < 100);

	__int64 nBytesRemain = fIsUnicode ? nOverlaySize - 4 : nOverlaySize;
	int nFilesLeft = fIsUnicode ? (int) numFiles : -1;
	if (!fIsUnicode) headerFile->SetPos(nOverlayStart);

	while (nBytesRemain > 0 && nFilesLeft != 0)
	{
		SfxFilePlainEntry entry = {0};
		__int64 entrySize = fIsUnicode ? ReadUnicodeEntry(headerFile, &entry) : ReadAnsiEntry(headerFile, &entry);
		if ((entrySize <= 0) || !entry.Path[0] || (entry.Size <= 0)) return false;

		nBytesRemain -= entrySize;
		m_vFiles.push_back(entry);

		if (nFilesLeft > 0) nFilesLeft--;
	}

	return true;
}

void ISSfx::ISPlainSfxFile::Close()
{
	if (m_pHeaderFile != nullptr)
	{
		delete m_pHeaderFile;
		m_pHeaderFile = nullptr;
	}

	m_vFiles.clear();
}

int ISSfx::ISPlainSfxFile::GetTotalFiles() const
{
	return (int) m_vFiles.size();
}

bool ISSfx::ISPlainSfxFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return false;

	const SfxFilePlainEntry &fileEntry = m_vFiles[itemIndex];

	wcscpy_s(itemInfo->Path, sizeof(itemInfo->Path) / sizeof(itemInfo->Path[0]), fileEntry.Path);
	itemInfo->Size = fileEntry.Size;

	return true;
}

#define COPY_BUFFER_SIZE 64*1024

int ISSfx::ISPlainSfxFile::ExtractFile( int itemIndex, CFileStream* targetFile, ExtractProcessCallbacks progressCtx )
{
	if (itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return CAB_EXTRACT_READ_ERR;

	const SfxFilePlainEntry &fileEntry = m_vFiles[itemIndex];

	char copyBuffer[COPY_BUFFER_SIZE];

	m_pHeaderFile->SetPos(fileEntry.StartOffset);
	__int64 nBytesRemain = fileEntry.Size;
	while (nBytesRemain > 0)
	{
		DWORD copySize = (DWORD) min(nBytesRemain, COPY_BUFFER_SIZE);
		if (!m_pHeaderFile->ReadBuffer(copyBuffer, copySize))
			return CAB_EXTRACT_READ_ERR;

		if (!progressCtx.FileProgress(progressCtx.signalContext, copySize))
			return CAB_EXTRACT_USER_ABORT;

		if (!targetFile->WriteBuffer(copyBuffer, copySize))
			return CAB_EXTRACT_WRITE_ERR;

		nBytesRemain -= copySize;
	}
		
	return CAB_EXTRACT_OK;
}

__int64 ISSfx::ISPlainSfxFile::ReadAnsiEntry( CFileStream* pFile, SfxFilePlainEntry *pEntry )
{
	__int64 filePos = pFile->GetPos();
	__int64 entrySize = 0;

	char dataBuf[1024] = {0};
	size_t readSize = 0;
	if (!pFile->ReadBufferAny(dataBuf, sizeof(dataBuf), &readSize))
		return -1;

	char* sName = dataBuf;
	char* sFullName = sName + strlen(sName) + 1;
	char* sVersion = sFullName + strlen(sFullName) + 1;
	char* sSize = sVersion + strlen(sVersion) + 1;

	long nSize = atol(sSize);
	entrySize = sSize - sName + strlen(sSize) + 1 + nSize;

	if (!pFile->SetPos(filePos + entrySize))
		return -1;

	MultiByteToWideChar(CP_ACP, 0, sFullName, -1, pEntry->Path, sizeof(pEntry->Path) / sizeof(pEntry->Path[0]));
	pEntry->StartOffset = filePos + (entrySize - nSize);
	pEntry->Size = nSize;

	return entrySize;
}

__int64 ISSfx::ISPlainSfxFile::ReadUnicodeEntry( CFileStream* pFile, SfxFilePlainEntry *pEntry )
{
	__int64 filePos = pFile->GetPos();
	__int64 entrySize = 0;

	wchar_t dataBuf[1024] = {0};
	size_t readSize = 0;
	if (!pFile->ReadBufferAny(dataBuf, sizeof(dataBuf), &readSize))
		return -1;

	wchar_t* sName = dataBuf;
	wchar_t* sFullName = sName + wcslen(sName) + 1;
	wchar_t* sVersion = sFullName + wcslen(sFullName) + 1;
	wchar_t* sSize = sVersion + wcslen(sVersion) + 1;

	long nSize = _wtol(sSize);
	entrySize = (sSize - sName + wcslen(sSize) + 1) * sizeof(wchar_t) + nSize;

	if (!pFile->SetPos(filePos + entrySize))
		return -1;

	wcscpy_s(pEntry->Path, sizeof(pEntry->Path) / sizeof(pEntry->Path[0]), sFullName);
	pEntry->StartOffset = filePos + (entrySize - nSize);
	pEntry->Size = nSize;

	return entrySize;
}
