#include "stdafx.h"
#include "IS5CabFile.h"
#include "Utils.h"

namespace IS5
{

#define GetFileDesc(dft, i) ( (FILEDESC*) (((LPBYTE)dft) + ((DWORD*)dft)[i]) )
#define GetString(ptr, ofs) ( (LPSTR) (((LPBYTE)ptr) + ((DWORD)ofs)) )
#define GetFileGroupDesc(ptr, fgt) ( (FILEGROUPDESC*) (((LPBYTE)ptr) + fgt) )
#define GetFileGroupEntry(ptr, ofs) ( (COMPFILEENTRY*) (((LPBYTE)ptr) + ofs) )
#define GetCompDesc(ptr, ofs) ( (COMPONENTDESC*) (((LPBYTE)ptr) + ofs) )
#define GetCompEntry(ptr, ofs) ( (COMPFILEENTRY*) (((LPBYTE)ptr) + ofs) )
#define GetSetupTypeDesc(ptr, ofs) ( (SETUPTYPEDESC*) (((LPBYTE)ptr) + ofs) )

IS5CabFile::IS5CabFile()
{
	m_pHeaderFile = nullptr;
	memset(&m_Header, 0, sizeof(m_Header));
	m_pCabDesc = NULL;
	m_pDFT = NULL;
	m_eCompression = Unknown;
}

IS5CabFile::~IS5CabFile()
{
	Close();
}

bool IS5CabFile::IsVersionCompatible( DWORD version )
{
	//NOTE: Some archives have version == 0, not sure why.
	//But they can be opened as version 5.
	DWORD nVer = GetMajorVersion(version);
	return (nVer == 5) || (nVer == 0);
}

int IS5CabFile::GetTotalFiles() const
{
	return (int) m_vValidFiles.size();
}

DWORD IS5CabFile::MajorVersion() const
{
	return GetMajorVersion(m_Header.Version);
}

bool IS5CabFile::GetFileInfo( int itemIndex, StorageItemInfo* itemInfo ) const
{
	if (!m_pCabDesc || !m_pDFT || itemIndex < 0 || itemIndex >= (int)m_vValidFiles.size())
		return false;

	DWORD fileIndex = m_vValidFiles[itemIndex];
	FILEDESC* pFileDesc = GetFileDesc(m_pDFT, m_pCabDesc->cDirs + fileIndex);

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
	itemInfo->PackedSize = pFileDesc->cbCompressed;
	itemInfo->Attributes = pFileDesc->Attrs;
	DosDateTimeToFileTime((WORD)pFileDesc->FatDate, (WORD)pFileDesc->FatTime, &itemInfo->ModificationTime);

	return true;
}

bool IS5CabFile::InternalOpen( CFileStream* headerFile )
{
	if (headerFile == INVALID_HANDLE_VALUE)
		return false;

	CABHEADER cabHeader = {0};
	CABDESC* cabDesc = NULL;
	DWORD* DFT = NULL;
	
	if (!headerFile->ReadBuffer(&cabHeader, sizeof(cabHeader)))
		return false;

	// Validate file
	
	if (cabHeader.Signature != CAB_SIG)
		return false;
	if (!IsVersionCompatible(cabHeader.Version))
		return false;
	if (cabHeader.NextVol != 0 && cabHeader.NextVol != 2)
		return false; // Not a first volume
	if (cabHeader.ofsCabDesc == 0)
		return false; // Not a header

	// Read CABDESC

	if (!headerFile->SetPos(cabHeader.ofsCabDesc))
		return false;

	cabDesc = (CABDESC*) malloc(cabHeader.cbCabDesc);
	if (cabDesc == NULL) return false;

	if (!headerFile->ReadBuffer(cabDesc, cabHeader.cbCabDesc))
	{
		free(cabDesc);
		return false;
	}

	// Read DFT

	if (!headerFile->SetPos(cabHeader.ofsCabDesc + cabDesc->ofsDFT))
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

	if (!headerFile->ReadBuffer(DFT, cabDesc->cbDFT))
	{
		free(cabDesc);
		free(DFT);
		return false;
	}

	// Check if there are some valid files inside

	for (DWORD i = 0; i < cabDesc->cFiles; i++)
	{
		FILEDESC* pFile = GetFileDesc(DFT, cabDesc->cDirs + i);
		if ((pFile->DescStatus & DESC_INVALID) || (pFile->ofsData == 0)) continue;

		m_vValidFiles.push_back(i);
	}

	if (m_vValidFiles.size() == 0)
	{
		free(cabDesc);
		free(DFT);
		return false;
	}

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

	// Store information for future use

	m_pCabDesc = cabDesc;
	m_pDFT = DFT;
	m_eCompression = Unknown;
	memcpy_s(&m_Header, sizeof(m_Header), &cabHeader, sizeof(cabHeader));

	return true;
}

void IS5CabFile::Close()
{
	if (m_pHeaderFile != nullptr)
	{
		delete m_pHeaderFile;
		m_pHeaderFile = nullptr;
	}
	FREE_NULL(m_pCabDesc);
	FREE_NULL(m_pDFT);

	for (auto it = m_vVolumes.begin(); it != m_vVolumes.end(); it++)
	{
		DataVolume* vol = *it;
		if (vol != NULL)
		{
			delete vol->FileHandle;
			delete vol;
		}
	}

	m_vVolumes.clear();
	m_vComponents.clear();
	m_vFileGroups.clear();
	m_vValidFiles.clear();

	m_sInfoFile.clear();
}

void IS5CabFile::GenerateInfoFile()
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
		sstr << "First volume    : " << groupDesc.FirstVolume << std::endl;
		sstr << "Last volume     : " << groupDesc.LastVolume << std::endl;
		sstr << std::endl;
	}
	
	sstr << "[Setup Components]" << std::endl;
	for (auto cit = m_vComponents.cbegin(); cit != m_vComponents.cend(); cit++)
	{
		DWORD nCompIndex = *cit;
		COMPONENTDESC* pCompDesc = GetCompDesc(m_pCabDesc, nCompIndex);
		sstr << "Name         : " << GetString(m_pCabDesc, pCompDesc->ofsName) << std::endl;
		sstr << "Display Name : " << GetString(m_pCabDesc, pCompDesc->ofsDispName) << std::endl;
		sstr << "Description  : " << GetString(m_pCabDesc, pCompDesc->ofsDescription) << std::endl;
		sstr << "Target Dir   : " << GetString(m_pCabDesc, pCompDesc->ofsTargetDir) << std::endl;
		sstr << std::endl;
	}
	
	sstr << "[Setup Types]" << std::endl;
	SETUPTYPEHEADER* pSTH = (SETUPTYPEHEADER*) ((LPBYTE)m_pCabDesc + m_pCabDesc->ofsSTypes);
	DWORD* pSetupTypes = (DWORD*) ((LPBYTE)m_pCabDesc + pSTH->ofsSTypeTab);
	for (DWORD i = 0; i < pSTH->cSTypes; i++)
	{
		SETUPTYPEDESC* pSTypeDesc = GetSetupTypeDesc(m_pCabDesc, pSetupTypes[i]);
		sstr << "Name         : " << GetString(m_pCabDesc, pSTypeDesc->ofsName) << std::endl;
		sstr << "Display Name : " << GetString(m_pCabDesc, pSTypeDesc->ofsDispName) << std::endl;
		sstr << "Description  : " << GetString(m_pCabDesc, pSTypeDesc->ofsDescription) << std::endl;
		sstr << std::endl;
	}

	std::string strData = sstr.str();
	m_sInfoFile = ConvertStrings(strData);
}

int IS5CabFile::ExtractFile( int itemIndex, CFileStream* targetFile, ExtractProcessCallbacks progressCtx )
{
	if (!m_pCabDesc || !m_pDFT || itemIndex < 0 || itemIndex >= (int)m_vValidFiles.size())
		return CAB_EXTRACT_READ_ERR;

	DWORD fileIndex = m_vValidFiles[itemIndex];
	FILEDESC* pFileDesc = GetFileDesc(m_pDFT, m_pCabDesc->cDirs + fileIndex);
	
	if (pFileDesc->DescStatus & DESC_INVALID)
		return CAB_EXTRACT_READ_ERR;

	int firstSearchVolume = -1, lastSearchVolume = -1;
	
	// Get file group for the file. From it we can find first/last volume indexes
	for (auto cit = m_vFileGroups.cbegin(); cit != m_vFileGroups.cend(); cit++)
	{
		const FILEGROUPDESC &groupDesc = *cit;
		if (fileIndex >= groupDesc.FirstFile && fileIndex <= groupDesc.LastFile)
		{
			firstSearchVolume = groupDesc.FirstVolume;
			lastSearchVolume = groupDesc.LastVolume;
			break;
		}
	}

	// Volume boundaries not found
	if (firstSearchVolume < 0 || lastSearchVolume < 0)
		return CAB_EXTRACT_READ_ERR;

	// Search for target volume
	DataVolume* volume = NULL;
	for (int i = firstSearchVolume; i <= lastSearchVolume; i++)
	{
		volume = OpenVolume((DWORD) i);
		if (volume == NULL)	break;

		if (fileIndex >= (int) volume->Header.FirstFile && fileIndex <= (int) volume->Header.LastFile)
			break;
		volume = NULL;
	}

	// Can not find proper volume file
	if (volume == NULL)
		return CAB_EXTRACT_READ_ERR;

	if (!volume->FileHandle->SetPos(pFileDesc->ofsData))
		return CAB_EXTRACT_READ_ERR;

	BYTE md5Sig[16] = {0};
	int extractResult = CAB_EXTRACT_OK;

	// Check if file is compressed or not
	if (pFileDesc->DescStatus & DESC_COMPRESSED)
	{
		//TODO: figure out this combo (if it ever exist)
		if (pFileDesc->DescStatus & DESC_ENCRYPTED)
			return CAB_EXTRACT_READ_ERR;

		if (m_eCompression == Unknown)
		{
			DetectCompression(pFileDesc, volume);
			volume->FileHandle->SetPos(pFileDesc->ofsData);
		}

		switch(m_eCompression)
		{
			case New:
				extractResult = UnpackFile(volume->FileHandle, targetFile, pFileDesc->cbExpanded, md5Sig, &progressCtx);
				break;
			case Old:
				extractResult = UnpackFileOld(volume->FileHandle, pFileDesc->cbCompressed, targetFile, pFileDesc->cbExpanded, md5Sig, &progressCtx);
				break;
			default:
				return CAB_EXTRACT_READ_ERR;
		}
	}
	else
	{
		extractResult = TransferFile(volume->FileHandle, targetFile, pFileDesc->cbExpanded, (pFileDesc->DescStatus & DESC_ENCRYPTED) != 0, md5Sig, &progressCtx);
	}

	return extractResult;
}

DataVolume* IS5CabFile::OpenVolume( DWORD volumeIndex )
{
	while (m_vVolumes.size() <= volumeIndex)
		m_vVolumes.push_back(NULL);

	DataVolume* volume = m_vVolumes[volumeIndex];

	if (volume == NULL)
	{
		wchar_t volumePath[4096] = {0};
		swprintf_s(volumePath, m_sCabPattern.c_str(), volumeIndex);

		CFileStream* pFile = CFileStream::Open(volumePath, true, false);
		if (pFile == nullptr) return NULL;

		CABHEADER header;
		if (!pFile->ReadBuffer(&header, sizeof(header))
			|| (header.Signature != m_Header.Signature)
			|| (header.Version != m_Header.Version)
			)
		{
			delete pFile;
			return NULL;
		}

		volume = new DataVolume();
		volume->VolumeIndex = volumeIndex;
		volume->FilePath = volumePath;
		volume->FileHandle = pFile;
		volume->Header = header;
		volume->FileSize = pFile->GetSize();

		m_vVolumes[volumeIndex] = volume;
	}

	return volume;
}

void IS5CabFile::DetectCompression( FILEDESC* fileDesc, DataVolume* fileVolume )
{
	if (!fileVolume || !fileDesc) return;

	DWORD totalLen = 0;
	DWORD nextOfs = fileDesc->ofsData;
	WORD blockSize;

	while (totalLen < fileDesc->cbCompressed)
	{
		if (!fileVolume->FileHandle->SetPos(nextOfs)) return;

		if (!fileVolume->FileHandle->ReadBuffer(&blockSize, sizeof(blockSize))) return;

		blockSize += sizeof(blockSize);
		nextOfs += blockSize;
		totalLen += blockSize;
	}

	m_eCompression = (totalLen == fileDesc->cbCompressed) ? New : Old;
}

} // namespace IS5
