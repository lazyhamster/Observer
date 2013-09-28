#include "stdafx.h"
#include "PEHelper.h"
#include "Utils.h"

bool FindFileOverlay(HANDLE hFile, __int64 &nOverlayStartOffset, __int64 &nOverlaySize)
{
	__int64 StartOffset = 0;
	__int64 DataSize = 0;

	__int64 FileSize = SizeOfFile(hFile);
	SeekFile(hFile, 0);

	// Read headers
	IMAGE_DOS_HEADER dosHeader;
	if (!ReadBuffer(hFile, &dosHeader, sizeof(dosHeader)) || (dosHeader.e_magic != 0x5A4D))
		return false;

	if (!SeekFile(hFile, dosHeader.e_lfanew + sizeof(IMAGE_NT_SIGNATURE)))
		return false;

	IMAGE_FILE_HEADER fileHeader;
	IMAGE_OPTIONAL_HEADER32 optnHeader;
	
	if (!ReadBuffer(hFile, &fileHeader, sizeof(fileHeader)) || !ReadBuffer(hFile, &optnHeader, sizeof(optnHeader)))
		return false;
	if (fileHeader.Machine != IMAGE_FILE_MACHINE_I386 || fileHeader.NumberOfSections == 0)
		return false;

	// Read sections list (save for debug data calc later)

	std::vector<IMAGE_SECTION_HEADER> vSections;

	__int64 maxLimit = 0;
	for (WORD i = 0; i < fileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER sectionHead;
		if (!ReadBuffer(hFile, &sectionHead, sizeof(sectionHead)))
			return false;

		__int64 limit = sectionHead.PointerToRawData + sectionHead.SizeOfRawData;
		if (limit > FileSize) return false; // Section data is messed up so exit
		if (limit > maxLimit) maxLimit = limit;

		vSections.push_back(sectionHead);
	}

	StartOffset = maxLimit;
	DataSize = FileSize - maxLimit;

	if (DataSize <= 0) return false;

	// Check debug data (should be after file ends and before overlay)

	IMAGE_DATA_DIRECTORY dbgDataDir = optnHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
	int numDbgEntries = dbgDataDir.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

	if (numDbgEntries > 0)
	{
		size_t dbgSectionIndex = -1;
		for (size_t i = 0; i < vSections.size(); i++)
		{
			const IMAGE_SECTION_HEADER &sect = vSections[i];
			if (sect.Misc.VirtualSize > 0 && dbgDataDir.VirtualAddress >= sect.VirtualAddress && dbgDataDir.VirtualAddress < sect.VirtualAddress + sect.Misc.VirtualSize)
			{
				dbgSectionIndex = i;
				break;
			}
		}

		// Strange debug data
		if (dbgSectionIndex < 0) return false;

		__int64 debugInfoFileOffset = vSections[dbgSectionIndex].PointerToRawData + (dbgDataDir.VirtualAddress - vSections[dbgSectionIndex].VirtualAddress);

		if (!SeekFile(hFile, debugInfoFileOffset))
			return false;

		for (int i = 0; i < numDbgEntries; i++)
		{
			IMAGE_DEBUG_DIRECTORY ddir;
			if (!ReadBuffer(hFile, &ddir, sizeof(ddir)))
				return false;

			if (ddir.PointerToRawData + ddir.SizeOfData > StartOffset)
			{
				DataSize -= ddir.PointerToRawData + ddir.SizeOfData - StartOffset;
				StartOffset = ddir.PointerToRawData + ddir.SizeOfData;
			}

			if (DataSize <= 0) break;
		}
	}

	if (DataSize <= 0) return false;

	// Decrement certificate size if present (always at the end of file)

	IMAGE_DATA_DIRECTORY certDataDir = optnHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
	DataSize -= certDataDir.Size;
	
	if (DataSize > 0)
	{
		nOverlayStartOffset = StartOffset;
		nOverlaySize = DataSize;

		return true;
	}
	
	return false;
}
