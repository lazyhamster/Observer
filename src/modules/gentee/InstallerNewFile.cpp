#include "stdafx.h"
#include "InstallerNewFile.h"
#include "InstallerDefs.h"
#include "modulecrt/PEHelper.h"
#include "gea/LZGE/lzge.h"

int64_t FindLaucherSig(AStream* stream)
{
	char searchBuf[32 * 1024];
	size_t readSize;

	stream->SetPos(0);
	stream->ReadBufferAny(searchBuf, sizeof(searchBuf), &readSize);
	for (size_t i = 0; i < readSize - sizeof(SIG_TEXT); i++)
	{
		if (memcmp(searchBuf + i, SIG_TEXT, sizeof(SIG_TEXT)) == 0)
			return i;
	}

	return -1;
}

bool ReadHeadData(AStream* stream, int64_t sigPos, lahead_data &data)
{
	memset(&data, 0, sizeof(data));

	lahead_v1 lh1;
	lahead_v2 lh2;

	stream->SetPos(sigPos);
	stream->Read(&lh1);
	if (lh1.offset == sigPos)
	{
		data.exesize = lh1.exesize;
		data.minsize = lh1.minsize;
		data.exeext = lh1.exeext;
		data.pack = 1;
		data.flags = lh1.flags;
		data.dllsize = lh1.dllsize;
		data.gesize = lh1.gesize;
		memcpy(data.extsize, lh1.extsize, sizeof(data.extsize));
		
		return true;
	}

	stream->SetPos(sigPos);
	stream->Read(&lh2);
	if (lh2.offset == sigPos)
	{
		data.exesize = lh2.exesize;
		data.minsize = lh2.minsize;
		data.exeext = lh2.exeext;
		data.pack = lh2.pack;
		data.flags = lh2.flags;
		data.dllsize = lh2.dllsize;
		data.gesize = lh2.gesize;
		memcpy(data.extsize, lh2.extsize, sizeof(data.extsize));

		return true;
	}

	return false;
}

CMemoryStream* UnpackToStream(unsigned char* buf, uint32_t &packedSize)
{
	uint32_t dataSize = *((uint32_t*) buf);
	CMemoryStream* content = new CMemoryStream(dataSize);

	pbyte unpackBuf = (pbyte) malloc(dataSize);

	slzge lz = {0};
	packedSize = lzge_decode(buf + sizeof(dataSize), unpackBuf, dataSize, &lz);
	content->WriteBuffer(unpackBuf, dataSize);
	
	free(unpackBuf);
	return content;
}

//////////////////////////////////////////////////////////////////////////

InstallerNewFile::InstallerNewFile()
{
	m_pStream = nullptr;
	m_pEmbedFiles = nullptr;
	m_pMainFiles = nullptr;
}

InstallerNewFile::~InstallerNewFile()
{
	Close();
}

bool InstallerNewFile::Open( AStream* inStream, int64_t startOffset, bool ownStream )
{
	CFileStream* src = dynamic_cast<CFileStream*>(inStream);
	if (!src) return false;
	
	// Find signature
	int64_t sigPos = FindLaucherSig(inStream);
	if (sigPos < 0) return false;

	lahead_data lh;
	if (!ReadHeadData(inStream, sigPos, lh))
		return false;

	if (inStream->GetSize() < lh.minsize)
		return false;

	if (!ReadPESectionFiles(inStream, lh.gesize, lh.dllsize))
		return false;

	int64_t overlayStart, overlaySize;
	if (!FindFileOverlay(inStream, overlayStart, overlaySize))
		return false;

	// Read embedded temp files
	AStream* resStream = new CMemoryStream();
	if (LoadPEResource(src->FilePath(), L"SETUP_TEMP", RT_RCDATA, resStream))
	{
		BaseGenteeFile* embFiles = new GeaFile();
		if (embFiles->Open(resStream))
		{
			m_pEmbedFiles = embFiles;
			for (int i = 0; i < embFiles->GetFilesCount(); i++)
			{
				InstallerFileDesc* ef = new InstallerFileDesc();
				ef->PathPrefix = L"#temp#";
				ef->ParentFile = embFiles;
				ef->ParentFileIndex = i;
				m_vFiles.push_back(ef);
			}
		}
		else
		{
			delete embFiles;
			delete resStream;
			
			return false;
		}
	}

	// Read main files list
	int64_t extOffset = overlayStart;
	for (uint8_t i = 0; i < lh.exeext; i++)
	{
		if (lh.extsize[i] == 0) continue;

		inStream->SetPos(extOffset);

		GeaFile* partFile = new GeaFile();
		if (partFile->Open(inStream, extOffset, false))
		{
			m_pMainFiles = partFile;
			for (int i = 0; i < partFile->GetFilesCount(); i++)
			{
				InstallerFileDesc* ef = new InstallerFileDesc();
				ef->PathPrefix = L"#setup#";
				ef->ParentFile = partFile;
				ef->ParentFileIndex = i;
				m_vFiles.push_back(ef);
			}
		}
		else
		{
			delete partFile;
		}

		extOffset += lh.extsize[i];
	}
	
	m_pStream = inStream;
	m_fStreamOwned = ownStream;
	return true;
}

void InstallerNewFile::Close()
{
	if (m_pMainFiles)
	{
		delete m_pMainFiles;
		m_pMainFiles = nullptr;
	}

	if (m_pEmbedFiles)
	{
		delete m_pEmbedFiles;
		m_pEmbedFiles = nullptr;
	}

	for (auto it = m_vFiles.begin(); it != m_vFiles.end(); it++)
	{
		delete *it;
	}
	m_vFiles.clear();

	BaseClose();
}

bool InstallerNewFile::GetFileDesc( int index, StorageItemInfo* desc )
{
	if (index >= 0 && index < (int) m_vFiles.size())
	{
		InstallerFileDesc* fd = m_vFiles[index];

		if (fd->ParentFile)
		{
			bool retVal = fd->ParentFile->GetFileDesc(fd->ParentFileIndex, desc);
			if (retVal)
			{
				std::wstring finalPath = fd->PathPrefix.length() > 0 ? fd->PathPrefix + L"\\" + desc->Path : desc->Path;
				wcscpy_s(desc->Path, sizeof(desc->Path) / sizeof(desc->Path[0]), finalPath.c_str());
				return true;
			}
		}
		else if (fd->Content)
		{
			wcscpy_s(desc->Path, sizeof(desc->Path) / sizeof(desc->Path[0]), fd->Path.c_str());
			desc->Size = fd->Content->GetSize();
			desc->PackedSize = fd->PackedSize;

			return true;
		}
	}
	return false;
}

GenteeExtractResult InstallerNewFile::ExtractFile( int index, AStream* dest, const char* password )
{
	if (index < 0 || index >= (int)m_vFiles.size())
		return Failure;

	InstallerFileDesc* fd = m_vFiles[index];

	if (fd->Content)
	{
		fd->Content->SetPos(0);
		dest->CopyFrom(fd->Content);
		return Success;
	}
	else if (fd->ParentFile && (fd->ParentFileIndex >= 0))
	{
		return fd->ParentFile->ExtractFile(fd->ParentFileIndex, dest, password);
	}
	
	return Failure;
}

void InstallerNewFile::GetFileTypeName( wchar_t* buf, size_t bufSize )
{
	wcscpy_s(buf, bufSize, L"CreateInstall Installer");
}

void InstallerNewFile::GetCompressionName( wchar_t* buf, size_t bufSize )
{
	wcscpy_s(buf, bufSize, L"LZGE/PPMD");
}

bool InstallerNewFile::ReadPESectionFiles( AStream* inStream, uint32_t scriptPackedSize, uint32_t dllPackedSize )
{
	int64_t sectionStart, sectionSize;
	if (!FindPESection(inStream, ".gentee", sectionStart, sectionSize))
		return false;

	BYTE* sectionBuf = (BYTE*) malloc((size_t) sectionSize);
	if (!inStream->SetPos(sectionStart) ||
		!inStream->ReadBuffer(sectionBuf, (size_t) sectionSize))
	{
		free(sectionBuf);
		return false;
	}

	uint32_t packedSize;

	// Read dll
	CMemoryStream* dllContent = UnpackToStream(sectionBuf, packedSize);
	if (dllPackedSize != packedSize + sizeof(uint32_t))
	{
		delete dllContent;
		return false;
	}

	InstallerFileDesc* dll = new InstallerFileDesc();
	dll->Path = L"gentee.dll";
	dll->PackedSize = packedSize;
	dll->Content = dllContent;
	m_vFiles.push_back(dll);

	// Read bytecode
	AStream* scriptContent = UnpackToStream(sectionBuf + dll->PackedSize + sizeof(uint32_t), packedSize);
	if (scriptPackedSize != packedSize + sizeof(uint32_t))
	{
		delete scriptContent;
		return false;
	}

	InstallerFileDesc* script = new InstallerFileDesc();
	script->Path = L"script.ge";
	script->PackedSize = packedSize;
	script->Content = scriptContent;
	m_vFiles.push_back(script);

	free(sectionBuf);
	return true;
}
