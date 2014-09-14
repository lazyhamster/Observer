#include "stdafx.h"
#include "ISUCabFile.h"
#include "Utils.h"

using namespace IS6;

namespace ISU
{

#define GetFileDesc(ptr, dft, i) ( ((FILEDESC*) (((BYTE*)dft) + ptr->ofsFilesDFT) ) + i )
#define GetString(ptr, ofs) ( (LPWSTR) (((LPBYTE)ptr) + ((DWORD)ofs)) )
#define GetFileGroupDesc(ptr, fgt) ( (FILEGROUPDESC*) (((LPBYTE)ptr) + fgt) )
#define GetFileGroupEntry(ptr, ofs) ( (COMPFILEENTRY*) (((LPBYTE)ptr) + ofs) )
#define GetCompDesc(ptr, ofs) ( (COMPONENTDESC*) (((LPBYTE)ptr) + ofs) )
#define GetCompEntry(ptr, ofs) ( (COMPFILEENTRY*) (((LPBYTE)ptr) + ofs) )
#define GetSetupTypeDesc(ptr, ofs) ( (SETUPTYPEDESC*) (((LPBYTE)ptr) + ofs) )

ISUCabFile::ISUCabFile()
{
	m_pHeaderFile = nullptr;
	memset(&m_Header, 0, sizeof(m_Header));
	m_pCabDesc = NULL;
	m_pDFT = NULL;
}

ISUCabFile::~ISUCabFile()
{
	Close();
}

bool ISUCabFile::IsVersionCompatible( DWORD version )
{
	DWORD major_version = GetMajorVersion(version);
	return major_version >= 17;
}

bool ISUCabFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (!m_pCabDesc || !m_pDFT || itemIndex < 0 || itemIndex >= (int)m_vValidFiles.size())
		return false;

	DWORD fileIndex = m_vValidFiles[itemIndex];
	FILEDESC* pFileDesc = GetFileDesc(m_pCabDesc, m_pDFT, fileIndex);

	wchar_t* pDir = GetString(m_pDFT, m_pDFT[pFileDesc->DirIndex]);
	wchar_t* pName = GetString(m_pDFT, pFileDesc->ofsName);

	if (!pDir && !pName)
		return false;

	// Find file group
	wchar_t* pFileGroupName = NULL;
	for (auto cit = m_vFileGroups.cbegin(); cit != m_vFileGroups.cend(); cit++)
	{
		const FILEGROUPDESC &groupDesc = *cit;
		if (fileIndex >= groupDesc.FirstFile && fileIndex <= groupDesc.LastFile)
		{
			pFileGroupName = GetString(m_pCabDesc, groupDesc.ofsName);
			break;
		}
	}
	wchar_t sGroupNameBuf[MAX_PATH] = {0};
	if (pFileGroupName && &pFileGroupName)
		swprintf_s(sGroupNameBuf, MAX_PATH, L"{%s}", pFileGroupName);

	// Create full path
	wchar_t fullName[MAX_PATH] = {0};
	CombinePath(fullName, MAX_PATH, 3, sGroupNameBuf, pDir, pName);

	// Fill result structure
	wcscpy_s(itemInfo->Path, STRBUF_SIZE(itemInfo->Path), fullName);
	itemInfo->Size = pFileDesc->cbExpanded;
	itemInfo->Attributes = pFileDesc->Attrs;
	DosDateTimeToFileTime((WORD)pFileDesc->FatDate, (WORD)pFileDesc->FatTime, &itemInfo->ModificationTime);

	return true;
}

void ISUCabFile::GenerateInfoFile()
{
	m_sInfoFile.clear();

	std::wstringstream sstr;

	sstr << L"[File Groups]" << std::endl;
	for (auto cit = m_vFileGroups.cbegin(); cit != m_vFileGroups.cend(); cit++)
	{
		const FILEGROUPDESC &groupDesc = *cit;
		wchar_t *pFileGroupName = GetString(m_pCabDesc, groupDesc.ofsName);
		sstr << L"Name            : \"" << pFileGroupName << "\"" << std::endl;
		sstr << L"Files           : " << groupDesc.LastFile - groupDesc.FirstFile + 1 << std::endl;
		sstr << L"Compressed size : " << groupDesc.cbCompressed << std::endl;
		sstr << L"Original size   : " << groupDesc.cbExpanded << std::endl;
		sstr << L"Target directory: " << GetString(m_pCabDesc, groupDesc.ofsTargetDir) << std::endl;
		sstr << std::endl;
	}

	sstr << L"[Setup Components]" << std::endl;
	for (auto cit = m_vComponents.cbegin(); cit != m_vComponents.cend(); cit++)
	{
		DWORD nCompIndex = *cit;
		COMPONENTDESC* pCompDesc = GetCompDesc(m_pCabDesc, nCompIndex);
		sstr << L"Name         : " << GetString(m_pCabDesc, pCompDesc->ofsName) << std::endl;

		wchar_t* pDispName = GetString(m_pCabDesc, pCompDesc->ofsDispName);
		if (pDispName && *pDispName)
			sstr << L"Display Name : " << pDispName << std::endl;

		sstr << L"Description  : " << GetString(m_pCabDesc, pCompDesc->ofsDescription) << std::endl;
		sstr << std::endl;
	}

	sstr << L"[Setup Types]" << std::endl;
	SETUPTYPEHEADER* pSTH = (SETUPTYPEHEADER*) ((LPBYTE)m_pCabDesc + m_pCabDesc->ofsSTypes);
	DWORD* pSetupTypes = (DWORD*) ((LPBYTE)m_pCabDesc + pSTH->ofsSTypeTab);
	for (DWORD i = 0; i < pSTH->cSTypes; i++)
	{
		SETUPTYPEDESC* pSTypeDesc = GetSetupTypeDesc(m_pCabDesc, pSetupTypes[i]);
		sstr << L"Name         : " << GetString(m_pCabDesc, pSTypeDesc->ofsName) << std::endl;

		wchar_t* pDispName = GetString(m_pCabDesc, pSTypeDesc->ofsDispName);
		if (pDispName && *pDispName)
			sstr << L"Display Name : " << pDispName << std::endl;

		sstr << L"Description  : " << GetString(m_pCabDesc, pSTypeDesc->ofsDescription) << std::endl;
		sstr << std::endl;
	}

	m_sInfoFile = sstr.str();
}

} // namespace ISU
