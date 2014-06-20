#include "stdafx.h"
#include "HLLib/Packages.h"

struct HLPackageTest
{
	HLPackageType ePackageType;
	hlUInt uiTestLength;
	hlByte lpTest[HL_DEFAULT_PACKAGE_TEST_BUFFER_SIZE];
};

static HLPackageTest lpPackageTests[] =
{
	{ HL_PACKAGE_BSP, 4, { 0x1e, 0x00, 0x00, 0x00 } },
	{ HL_PACKAGE_GCF, 8, { 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 } },
	{ HL_PACKAGE_PAK, 4, { 'P', 'A', 'C', 'K' } },
	{ HL_PACKAGE_SGA, 8, { '_', 'A', 'R', 'C', 'H', 'I', 'V', 'E' } },
	{ HL_PACKAGE_VBSP, 4, { 'V', 'B', 'S', 'P' } },
	{ HL_PACKAGE_VPK, 4, { 0x34, 0x12, 0xaa, 0x55 } },
	{ HL_PACKAGE_WAD, 4, { 'W', 'A', 'D', '3' } },
	{ HL_PACKAGE_XZP, 4, { 'p', 'i', 'Z', 'x' } },
	{ HL_PACKAGE_NONE, 0, { } }
};

static HLPackageType hlGetPackageTypeFromMemory(const hlVoid *lpBuffer, hlUInt uiBufferSize)
{
	if(uiBufferSize == 0)
	{
		return HL_PACKAGE_NONE;
	}

	for(HLPackageTest *pPackageTest = lpPackageTests; pPackageTest->ePackageType != HL_PACKAGE_NONE; pPackageTest++)
	{
		if(pPackageTest->uiTestLength <= uiBufferSize && memcmp(lpBuffer, pPackageTest->lpTest, pPackageTest->uiTestLength) == 0)
		{
			return pPackageTest->ePackageType;
		}
	}

	return HL_PACKAGE_NONE;
}

HLPackageType GetPackageType(const wchar_t* filepath)
{
	HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return HL_PACKAGE_NONE;

	HLPackageType type = HL_PACKAGE_NONE;
	
	hlByte readBuf[128] = {0};
	DWORD dwRead;

	if (ReadFile(hFile, readBuf, sizeof(readBuf), &dwRead, NULL) && dwRead > 0)
	{
		type = hlGetPackageTypeFromMemory(readBuf, sizeof(readBuf));
	}

	CloseHandle(hFile);
	return type;
}

HLLib::CPackage* CreatePackage(HLPackageType ePackageType)
{
	HLLib::CPackage* pNewPackage = NULL;
	
	switch(ePackageType)
	{
		case HL_PACKAGE_BSP:
			pNewPackage = new HLLib::CBSPFile();
			break;
		case HL_PACKAGE_GCF:
			pNewPackage = new HLLib::CGCFFile();
			break;
		case HL_PACKAGE_PAK:
			pNewPackage = new HLLib::CPAKFile();
			break;
		case HL_PACKAGE_SGA:
			pNewPackage = new HLLib::CSGAFile();
			break;
		case HL_PACKAGE_VBSP:
			pNewPackage = new HLLib::CVBSPFile();
			break;
		case HL_PACKAGE_VPK:
			pNewPackage = new HLLib::CVPKFile();
			break;
		case HL_PACKAGE_WAD:
			pNewPackage = new HLLib::CWADFile();
			break;
		case HL_PACKAGE_XZP:
			pNewPackage = new HLLib::CXZPFile();
			break;
	}

	return pNewPackage;
}
