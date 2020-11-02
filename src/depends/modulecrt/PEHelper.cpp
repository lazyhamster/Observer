#include "stdafx.h"
#include "PEHelper.h"

bool FindFileOverlay(AStream *inStream, int64_t &nOverlayStartOffset, int64_t &nOverlaySize)
{
	int64_t StartOffset = 0;
	int64_t DataSize = 0;
	int64_t FileSize = inStream->GetSize();

	inStream->SetPos(0);

	// Read headers
	IMAGE_DOS_HEADER dosHeader;
	if (!inStream->ReadBuffer(&dosHeader, sizeof(dosHeader)) || (dosHeader.e_magic != 0x5A4D))
		return false;

	if (!inStream->SetPos(dosHeader.e_lfanew + sizeof(IMAGE_NT_SIGNATURE)))
		return false;

	IMAGE_FILE_HEADER fileHeader;
	IMAGE_OPTIONAL_HEADER32 optnHeader;

	if (!inStream->ReadBuffer(&fileHeader, sizeof(fileHeader)) || !inStream->ReadBuffer(&optnHeader, sizeof(optnHeader)))
		return false;
	if (fileHeader.Machine != IMAGE_FILE_MACHINE_I386 || fileHeader.NumberOfSections == 0)
		return false;

	// Read sections list (save for debug data calc later)

	std::vector<IMAGE_SECTION_HEADER> vSections;

	int64_t maxLimit = 0;
	for (WORD i = 0; i < fileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER sectionHead;
		if (!inStream->ReadBuffer(&sectionHead, sizeof(sectionHead)))
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
		int dbgSectionIndex = -1;
		for (size_t i = 0; i < vSections.size(); i++)
		{
			const IMAGE_SECTION_HEADER &sect = vSections[i];
			if (sect.Misc.VirtualSize > 0 && dbgDataDir.VirtualAddress >= sect.VirtualAddress && dbgDataDir.VirtualAddress < sect.VirtualAddress + sect.Misc.VirtualSize)
			{
				dbgSectionIndex = static_cast<int>(i);
				break;
			}
		}

		// Strange debug data
		if (dbgSectionIndex < 0) return false;

		__int64 debugInfoFileOffset = vSections[dbgSectionIndex].PointerToRawData + (dbgDataDir.VirtualAddress - vSections[dbgSectionIndex].VirtualAddress);

		if (!inStream->SetPos(debugInfoFileOffset))
			return false;

		for (int i = 0; i < numDbgEntries; i++)
		{
			IMAGE_DEBUG_DIRECTORY ddir;
			if (!inStream->ReadBuffer(&ddir, sizeof(ddir)))
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

std::string GetManifest(const wchar_t* libraryPath)
{
	HMODULE hMod = LoadLibraryEx(libraryPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (hMod == NULL) return "";

	std::string manifestText;

	HRSRC hRsc = FindResource(hMod, MAKEINTRESOURCE(1), RT_MANIFEST);
	if (hRsc != NULL)
	{
		HGLOBAL resData = LoadResource(hMod, hRsc);
		DWORD resSize = SizeofResource(hMod, hRsc);

		if (resData && resSize)
		{
			char* pManifestData = (char*) LockResource(resData);
			manifestText.append(pManifestData, resSize);
			UnlockResource(pManifestData);
		}
		FreeResource(resData);
	}

	FreeLibrary(hMod);
	return manifestText;
}

bool FindPESection(AStream *inStream, const char* szSectionName, int64_t &nSectionStartOffset, int64_t &nSectionSize)
{
	int64_t StartOffset = 0;
	int64_t DataSize = 0;
	int64_t FileSize = inStream->GetSize();

	inStream->SetPos(0);

	// Read headers
	IMAGE_DOS_HEADER dosHeader;
	if (!inStream->ReadBuffer(&dosHeader, sizeof(dosHeader)) || (dosHeader.e_magic != 0x5A4D))
		return false;

	if (!inStream->SetPos(dosHeader.e_lfanew + sizeof(IMAGE_NT_SIGNATURE)))
		return false;

	IMAGE_FILE_HEADER fileHeader;
	IMAGE_OPTIONAL_HEADER32 optnHeader;

	if (!inStream->ReadBuffer(&fileHeader, sizeof(fileHeader)) || !inStream->ReadBuffer(&optnHeader, sizeof(optnHeader)))
		return false;
	if (fileHeader.Machine != IMAGE_FILE_MACHINE_I386 || fileHeader.NumberOfSections == 0)
		return false;

	int64_t maxLimit = 0;
	for (WORD i = 0; i < fileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER sectionHead;
		if (!inStream->ReadBuffer(&sectionHead, sizeof(sectionHead)))
			return false;

		__int64 limit = sectionHead.PointerToRawData + sectionHead.SizeOfRawData;
		if (limit > FileSize) return false; // Section data is messed up so exit
		if (limit > maxLimit) maxLimit = limit;

		if (_stricmp((char*)sectionHead.Name, szSectionName) == 0)
		{
			nSectionStartOffset = sectionHead.PointerToRawData;
			nSectionSize = sectionHead.SizeOfRawData;
			return true;
		}
	}

	return false;
}

bool LoadPEResource( const wchar_t* exePath, const wchar_t* rcName, const wchar_t* rcType, AStream *destStream )
{
	HMODULE hMod = LoadLibraryEx(exePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (hMod == NULL) return false;

	bool ret = false;

	HRSRC hRsc = FindResource(hMod,rcName, rcType);
	if (hRsc != NULL)
	{
		HGLOBAL resData = LoadResource(hMod, hRsc);
		DWORD resSize = SizeofResource(hMod, hRsc);

		if (resData && resSize)
		{
			void* pRes = LockResource(resData);
			destStream->WriteBuffer(pRes, resSize);
			UnlockResource(pRes);
			ret = true;
		}
		FreeResource(resData);
	}

	FreeLibrary(hMod);
	return ret;
}
