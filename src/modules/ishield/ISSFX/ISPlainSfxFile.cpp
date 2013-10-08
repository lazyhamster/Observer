#include "stdafx.h"
#include "ISPlainSfxFile.h"
#include "Utils.h"
#include "PEHelper.h"

ISSfx::ISPlainSfxFile::ISPlainSfxFile()
{
	m_hHeaderFile = INVALID_HANDLE_VALUE;
}

ISSfx::ISPlainSfxFile::~ISPlainSfxFile()
{
	Close();
}

bool ISSfx::ISPlainSfxFile::InternalOpen( HANDLE headerFile )
{
	// Since this type of storage does not have signature we will rely on manifest
	if (!CheckManifest())
		return false;

	__int64 nOverlayStart, nOverlaySize;

	if (!FindFileOverlay(headerFile, nOverlayStart, nOverlaySize) || nOverlaySize < 2048)  // 2048 is a random number
		return false;

	SeekFile(headerFile, nOverlayStart);

	// Unicode version has number of files at the start
	// So if first 4 bytes produce reasonable number then let's assume it is unicode
	DWORD numFiles;
	ReadBuffer(headerFile, &numFiles, sizeof(numFiles));
	bool fIsUnicode = (numFiles < 100);

	__int64 nBytesRemain = fIsUnicode ? nOverlaySize - 4 : nOverlaySize;
	int nFilesLeft = fIsUnicode ? (int) numFiles : -1;
	if (!fIsUnicode) SeekFile(headerFile, nOverlayStart);

	while (nBytesRemain > 0 && nFilesLeft != 0)
	{
		SfxFilePlainEntry entry = {0};
		__int64 entrySize = fIsUnicode ? ReadUnicodeEntry(headerFile, &entry) : ReadAnsiEntry(headerFile, &entry);
		if (entrySize <= 0) return false;

		nBytesRemain -= entrySize;
		m_vFiles.push_back(entry);

		if (nFilesLeft > 0) nFilesLeft--;
	}

	return true;
}

void ISSfx::ISPlainSfxFile::Close()
{
	if (m_hHeaderFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHeaderFile);
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

int ISSfx::ISPlainSfxFile::ExtractFile( int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx )
{
	if (itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return CAB_EXTRACT_READ_ERR;

	const SfxFilePlainEntry &fileEntry = m_vFiles[itemIndex];

	char copyBuffer[COPY_BUFFER_SIZE];

	SeekFile(m_hHeaderFile, fileEntry.StartOffset);
	__int64 nBytesRemain = fileEntry.Size;
	while (nBytesRemain > 0)
	{
		DWORD copySize = (DWORD) min(nBytesRemain, COPY_BUFFER_SIZE);
		if (!ReadBuffer(m_hHeaderFile, copyBuffer, copySize))
			return CAB_EXTRACT_READ_ERR;

		if (!progressCtx.FileProgress(progressCtx.signalContext, copySize))
			return CAB_EXTRACT_USER_ABORT;

		if (!WriteBuffer(targetFile, copyBuffer, copySize))
			return CAB_EXTRACT_WRITE_ERR;

		nBytesRemain -= copySize;
	}
		
	return CAB_EXTRACT_OK;
}

bool ISSfx::ISPlainSfxFile::CheckManifest()
{
	HMODULE hMod = LoadLibraryEx(m_sHeaderFilePath.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (hMod == NULL) return false;

	bool fIsSetup = false;

	HRSRC hRsc = FindResource(hMod, MAKEINTRESOURCE(1), RT_MANIFEST);
	if (hRsc != NULL)
	{
		HGLOBAL resData = LoadResource(hMod, hRsc);
		DWORD resSize = SizeofResource(hMod, hRsc);

		if (resData && resSize)
		{
			char* pManifestData = (char*) LockResource(resData);
			fIsSetup = strstr(pManifestData, "InstallShield.Setup") != NULL;
			UnlockResource(pManifestData);
		}
		FreeResource(resData);
	}
	
	FreeLibrary(hMod);
	return fIsSetup;
}

__int64 ISSfx::ISPlainSfxFile::ReadAnsiEntry( HANDLE hFile, SfxFilePlainEntry *pEntry )
{
	__int64 filePos = FilePos(hFile);
	__int64 entrySize = 0;

	char dataBuf[1024] = {0};
	DWORD readSize = 0;
	if (!ReadBuffer(hFile, dataBuf, sizeof(dataBuf), &readSize))
		return -1;

	char* sName = dataBuf;
	char* sFullName = sName + strlen(sName) + 1;
	char* sVersion = sFullName + strlen(sFullName) + 1;
	char* sSize = sVersion + strlen(sVersion) + 1;

	long nSize = atol(sSize);
	entrySize = sSize - sName + strlen(sSize) + 1 + nSize;

	if (!SeekFile(hFile, filePos + entrySize))
		return -1;

	MultiByteToWideChar(CP_ACP, 0, sFullName, -1, pEntry->Path, sizeof(pEntry->Path) / sizeof(pEntry->Path[0]));
	pEntry->StartOffset = filePos + (entrySize - nSize);
	pEntry->Size = nSize;

	return entrySize;
}

__int64 ISSfx::ISPlainSfxFile::ReadUnicodeEntry( HANDLE hFile, SfxFilePlainEntry *pEntry )
{
	__int64 filePos = FilePos(hFile);
	__int64 entrySize = 0;

	wchar_t dataBuf[1024] = {0};
	DWORD readSize = 0;
	if (!ReadBuffer(hFile, dataBuf, sizeof(dataBuf), &readSize))
		return -1;

	wchar_t* sName = dataBuf;
	wchar_t* sFullName = sName + wcslen(sName) + 1;
	wchar_t* sVersion = sFullName + wcslen(sFullName) + 1;
	wchar_t* sSize = sVersion + wcslen(sVersion) + 1;

	long nSize = _wtol(sSize);
	entrySize = (sSize - sName + wcslen(sSize) + 1) * sizeof(wchar_t) + nSize;

	if (!SeekFile(hFile, filePos + entrySize))
		return -1;

	wcscpy_s(pEntry->Path, sizeof(pEntry->Path) / sizeof(pEntry->Path[0]), sFullName);
	pEntry->StartOffset = filePos + (entrySize - nSize);
	pEntry->Size = nSize;

	return entrySize;
}
