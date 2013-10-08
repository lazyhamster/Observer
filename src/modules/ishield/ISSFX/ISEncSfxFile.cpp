#include "stdafx.h"
#include "ISEncSfxFile.h"
#include "Utils.h"
#include "PEHelper.h"

ISSfx::ISEncSfxFile::ISEncSfxFile()
{
	m_hHeaderFile = INVALID_HANDLE_VALUE;
}

ISSfx::ISEncSfxFile::~ISEncSfxFile()
{
	Close();
}

bool ISSfx::ISEncSfxFile::InternalOpen( HANDLE headerFile )
{
	__int64 nOverlayStart, nOverlaySize;

	if (!FindFileOverlay(headerFile, nOverlayStart, nOverlaySize))
		return false;

	SeekFile(headerFile, nOverlayStart);

	char Sig[14];
	if (!ReadBuffer(headerFile, Sig, sizeof(Sig)) || strcmp(Sig, "InstallShield"))
		return false;

	DWORD NumFiles;
	if (!ReadBuffer(headerFile, &NumFiles, sizeof(NumFiles)) || NumFiles == 0)
		return false;

	if (!SeekFile(headerFile, FilePos(headerFile) + 0x1C))
		return false;

	for (DWORD i = 0; i < NumFiles; i++)
	{
		SfxFileEntry fe;
		ReadBuffer(headerFile, &fe.Header, sizeof(fe.Header));
		fe.StartOffset = FilePos(headerFile);
		m_vFiles.push_back(fe);

		if ((fe.StartOffset + fe.Header.CompressedSize > nOverlayStart + nOverlaySize) || !SeekFile(headerFile, fe.StartOffset + fe.Header.CompressedSize))
			return false;
	}

	m_hHeaderFile = headerFile;
	
	return m_vFiles.size() > 0;
}

void ISSfx::ISEncSfxFile::Close()
{
	if (m_hHeaderFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHeaderFile);
	}

	m_vFiles.clear();
}

int ISSfx::ISEncSfxFile::GetTotalFiles() const
{
	return (int) m_vFiles.size();
}

bool ISSfx::ISEncSfxFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (itemIndex < 0 || itemIndex >= (int)m_vFiles.size())
		return false;

	const SfxFileEntry &fileEntry = m_vFiles[itemIndex];

	// Fill result structure
	MultiByteToWideChar(CP_ACP, 0, fileEntry.Header.Name, -1, itemInfo->Path, STRBUF_SIZE(itemInfo->Path));
	itemInfo->Size = fileEntry.Header.CompressedSize;

	//TODO: find uncompressed size

	return true;
}

int ISSfx::ISEncSfxFile::ExtractFile( int itemIndex, HANDLE targetFile, ExtractProcessCallbacks progressCtx )
{
	return CAB_EXTRACT_READ_ERR;
}
