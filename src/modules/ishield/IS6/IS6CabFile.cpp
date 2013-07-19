#include "stdafx.h"
#include "IS6CabFile.h"
#include "Utils.h"

namespace IS6
{

#define GetFileDesc(ptr, dft, i) ( ((FILEDESC*) (((BYTE*)dft) + ptr->ofsFilesDFT) ) + i )
#define GetString(ptr, ofs) ( (LPSTR) (((LPBYTE)ptr) + ((DWORD)ofs)) )
#define GetFileGroupDesc(ptr, fgt) ( (FILEGROUPDESC*) (((LPBYTE)ptr) + fgt) )
#define GetFileGroupEntry(ptr, ofs) ( (COMPFILEENTRY*) (((LPBYTE)ptr) + ofs) )
#define GetCompDesc(ptr, ofs) ( (COMPONENTDESC*) (((LPBYTE)ptr) + ofs) )
#define GetCompEntry(ptr, ofs) ( (COMPFILEENTRY*) (((LPBYTE)ptr) + ofs) )
#define GetSetupTypeDesc(ptr, ofs) ( (SETUPTYPEDESC*) (((LPBYTE)ptr) + ofs) )

IS6CabFile::IS6CabFile()
{
	m_hHeaderFile = INVALID_HANDLE_VALUE;
	memset(&m_Header, 0, sizeof(m_Header));
	m_pCabDesc = NULL;
	m_pDFT = NULL;
}

IS6CabFile::~IS6CabFile()
{
	Close();
}

bool IS6CabFile::IsVersionCompatible( DWORD version )
{
	DWORD major_version = GetMajorVersion(version);
	return major_version >= 6 && major_version <= 15;
}

int IS6CabFile::GetTotalFiles() const
{
	return (int) m_vValidFiles.size();
}

bool IS6CabFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (!m_pCabDesc || !m_pDFT || itemIndex < 0 || itemIndex >= (int)m_vValidFiles.size())
		return false;

	DWORD fileIndex = m_vValidFiles[itemIndex];
	FILEDESC* pFileDesc = GetFileDesc(m_pCabDesc, m_pDFT, fileIndex);

	char* pDir = GetString(m_pDFT, m_pDFT[pFileDesc->DirIndex]);
	char* pName = GetString(m_pDFT, pFileDesc->ofsName);

	if (!pDir && !pName)
		return false;

	// Find file group
	char* pFileGroupName = NULL;
	for (auto cit = m_vFileGroups.cbegin(); cit != m_vFileGroups.cend(); cit++)
	{
		const FILEGROUPDESC &groupDesc = *cit;
		if (fileIndex >= groupDesc.FirstFile && fileIndex <= groupDesc.LastFile)
		{
			pFileGroupName = GetString(m_pCabDesc, groupDesc.ofsName);
			break;
		}
	}
	char sGroupNameBuf[MAX_PATH] = {0};
	if (pFileGroupName && &pFileGroupName)
		sprintf_s(sGroupNameBuf, MAX_PATH, "{%s}", pFileGroupName);

	// Create full path
	char fullName[MAX_PATH] = {0};
	CombinePath(fullName, MAX_PATH, 3, sGroupNameBuf, pDir, pName);

	// Fill result structure
	MultiByteToWideChar(CP_ACP, 0, fullName, -1, itemInfo->Path, STRBUF_SIZE(itemInfo->Path));
	itemInfo->Size = pFileDesc->cbExpanded;
	itemInfo->Attributes = pFileDesc->Attrs;
	DosDateTimeToFileTime((WORD)pFileDesc->FatDate, (WORD)pFileDesc->FatTime, &itemInfo->ModificationTime);

	return true;
}

bool IS6CabFile::Open( HANDLE headerFile )
{
	if (headerFile == INVALID_HANDLE_VALUE)
		return false;

	CABHEADER cabHeader = {0};
	CABDESC* cabDesc = NULL;
	DWORD* DFT = NULL;

	if (!ReadBuffer(headerFile, &cabHeader, sizeof(cabHeader)))
		return false;

	// Validate file

	if (cabHeader.Signature != CAB_SIG)
		return false;
	if (!IsVersionCompatible(cabHeader.Version))
		return false;
	if (cabHeader.ofsCabDesc == 0)
		return false; // Not a header

	// Read CABDESC

	if (!SeekFile(headerFile, cabHeader.ofsCabDesc))
		return false;

	cabDesc = (CABDESC*) malloc(cabHeader.cbCabDesc);
	if (cabDesc == NULL) return false;

	if (!ReadBuffer(headerFile, cabDesc, cabHeader.cbCabDesc))
	{
		free(cabDesc);
		return false;
	}

	// Read DFT

	if (!SeekFile(headerFile, cabHeader.ofsCabDesc + cabDesc->ofsDFT))
	{
		free(cabDesc);
		return false;
	}

	DFT = (DWORD*) malloc(cabDesc->cbDFT);
	if (DFT == NULL)
	{
		free(cabDesc);
		return false;
	}

	if (!ReadBuffer(headerFile, DFT, cabDesc->cbDFT))
	{
		free(cabDesc);
		free(DFT);
		return false;
	}

	// Check if there are some valid files inside

	for (DWORD i = 0; i < cabDesc->cFiles; i++)
	{
		FILEDESC* pFile = GetFileDesc(cabDesc, DFT, i);
		if (pFile->DescStatus & DESC_INVALID)
			continue;

		m_vValidFiles.push_back(i);
	}

	if (m_vValidFiles.size() == 0)
	{
		free(cabDesc);
		free(DFT);
		return false;
	}

	// Store information for future use

	m_hHeaderFile = headerFile;
	m_pCabDesc = cabDesc;
	m_pDFT = DFT;
	memcpy_s(&m_Header, sizeof(m_Header), &cabHeader, sizeof(cabHeader));

	// Calculate some data

	m_vFileGroups.clear();
	for (int i = 0; i < CFG_MAX; i++)
	{
		DWORD ofsEntry = cabDesc->ofsFileGroup[i];
		while (ofsEntry != NULL)
		{
			COMPFILEENTRY* FGE = GetFileGroupEntry(cabDesc, ofsEntry);
			ofsEntry = FGE->ofsNext;

			FILEGROUPDESC* FGD = GetFileGroupDesc(cabDesc, FGE->ofsDesc);
			m_vFileGroups.push_back(*FGD);
		}
	}

	m_vComponents.clear();
	for (int i = 0; i < CFG_MAX; i++)
	{
		DWORD ofsEntry = cabDesc->ofsComponent[i];
		while (ofsEntry != 0)
		{
			COMPFILEENTRY* CPE = GetCompEntry(cabDesc, ofsEntry);
			ofsEntry = CPE->ofsNext;
			m_vComponents.push_back(CPE->ofsDesc);
		}
	}

	GenerateInfoFile();
	return true;
}

void IS6CabFile::Close()
{
	if (m_hHeaderFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHeaderFile);
	}
	FREE_NULL(m_pCabDesc);
	FREE_NULL(m_pDFT);

	m_vComponents.clear();
	m_vFileGroups.clear();
	m_vValidFiles.clear();

	m_sInfoFile.clear();
}

bool IS6CabFile::ExtractFile( int itemIndex, HANDLE targetFile ) const
{
	if (!m_pCabDesc || !m_pDFT || itemIndex < 0 || itemIndex >= (int)m_vValidFiles.size())
		return false;

	DWORD fileIndex = m_vValidFiles[itemIndex];
	
	//TODO: implement

	return false;
}

void IS6CabFile::GenerateInfoFile()
{
	m_sInfoFile.clear();

	std::stringstream sstr;

	sstr << "[File Groups]" << std::endl;
	for (auto cit = m_vFileGroups.cbegin(); cit != m_vFileGroups.cend(); cit++)
	{
		const FILEGROUPDESC &groupDesc = *cit;
		char *pFileGroupName = GetString(m_pCabDesc, groupDesc.ofsName);
		sstr << "Name            : \"" << pFileGroupName << "\"" << std::endl;
		sstr << "Files           : " << groupDesc.LastFile - groupDesc.FirstFile + 1 << std::endl;
		sstr << "Compressed size : " << groupDesc.cbCompressed << std::endl;
		sstr << "Original size   : " << groupDesc.cbExpanded << std::endl;
		sstr << std::endl;
	}

	sstr << "[Setup Components]" << std::endl;
	for (auto cit = m_vComponents.cbegin(); cit != m_vComponents.cend(); cit++)
	{
		DWORD nCompIndex = *cit;
		COMPONENTDESC* pCompDesc = GetCompDesc(m_pCabDesc, nCompIndex);
		sstr << "Name         : " << GetString(m_pCabDesc, pCompDesc->ofsName) << std::endl;

		char* pDispName = GetString(m_pCabDesc, pCompDesc->ofsDispName);
		if (pDispName && *pDispName)
			sstr << "Display Name : " << pDispName << std::endl;

		char* unk1 = GetString(m_pCabDesc, pCompDesc->ofsUnkStr1);

		sstr << "Description  : " << GetString(m_pCabDesc, pCompDesc->ofsDescription) << std::endl;
		sstr << std::endl;
	}

	sstr << "[Setup Types]" << std::endl;
	SETUPTYPEHEADER* pSTH = (SETUPTYPEHEADER*) ((LPBYTE)m_pCabDesc + m_pCabDesc->ofsSTypes);
	DWORD* pSetupTypes = (DWORD*) ((LPBYTE)m_pCabDesc + pSTH->ofsSTypeTab);
	for (DWORD i = 0; i < pSTH->cSTypes; i++)
	{
		SETUPTYPEDESC* pSTypeDesc = GetSetupTypeDesc(m_pCabDesc, pSetupTypes[i]);
		sstr << "Name         : " << GetString(m_pCabDesc, pSTypeDesc->ofsName) << std::endl;

		char* pDispName = GetString(m_pCabDesc, pSTypeDesc->ofsDispName);
		if (pDispName && *pDispName)
			sstr << "Display Name : " << pDispName << std::endl;

		sstr << "Description  : " << GetString(m_pCabDesc, pSTypeDesc->ofsDescription) << std::endl;
		sstr << std::endl;
	}

	std::string strData = sstr.str();
	int numChars = MultiByteToWideChar(CP_ACP, 0, strData.c_str(), -1, NULL, 0);
	size_t bufSize = (numChars + 1) * sizeof(wchar_t);
	wchar_t* buf = (wchar_t*) malloc(bufSize);
	memset(buf, 0, bufSize);
	MultiByteToWideChar(CP_ACP, 0, strData.c_str(), -1, buf, numChars + 1);
	m_sInfoFile = buf;
	free(buf);
}

}// namespace IS6
