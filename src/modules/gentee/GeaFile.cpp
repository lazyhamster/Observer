#include "stdafx.h"
#include "GeaFile.h"
#include "gea.h"
#include "gea/LZGE/lzge.h"
#include "gea/PPMD/endeppmd.h"
#include "gea/common/crc.h"

#define CRC_SEED 0xFFFFFFFF

extern "C"
{
dword STDCALL ppmd_decode( pbyte in, dword size, pbyte out, dword outsize, psppmd ppmd );
};

static void gea_protect(uint8_t* buf, size_t size, const char* passwd)
{
	//TODO: implement
}

GeaFile::GeaFile()
{
	m_pStream = nullptr;
	m_nBlockSize = 0;
	m_nSolidSize = 0;
	m_sPattern = "";
	m_pOutBuffer = nullptr;
	m_nOutBufferDataSize = 0;
}

GeaFile::~GeaFile()
{
	Close();
}

bool GeaFile::Open( AStream* inStream )
{
	if (inStream->GetSize() < sizeof(geavolume) + sizeof(geahead))
		return false;

	geavolume vol;
	geahead head;
	
	inStream->SetPos(0);
	inStream->ReadBuffer(&vol, sizeof(vol));
	inStream->ReadBuffer(&head, sizeof(head));

	if (strncmp(vol.name, "GEA", 4) != 0)
	{
		// Invalid magic
		return false;
	}

	if (head.majorver != GEA_MAJOR || head.minorver != GEA_MINOR)
	{
		// Invalid version
		return false;
	}

	//TODO: check how to make this work properly on installers
	//if (inStream->GetSize() < head.geasize)
	//{
		// Invalid file size
		//return false;
	//}

	m_sPattern = ReadGeaString(inStream);

	if (head.flags & GEAH_PASSWORD)
	{
		uint16_t passCount;
		uint32_t passCrc;

		inStream->Read(&passCount);
		for (uint16_t i = 0; i < passCount; i++)
		{
			inStream->Read(&passCrc);
			m_vPasswordCRC.push_back(passCrc);
			m_vPasswords.push_back("");
		}
	}

	CMemoryStream infoStream(head.infosize);

	if (head.flags & GEAH_COMPRESS)
	{
		pbyte compBuf = (pbyte) malloc(head.infosize);
		pbyte decompBuf = (pbyte) malloc(head.infosize);

		inStream->ReadBufferAny(compBuf, head.infosize, nullptr);
		
		slzge lz = {0};
		dword compSize = lzge_decode(compBuf, decompBuf, head.infosize, &lz);

		infoStream.WriteBuffer(decompBuf, head.infosize);

		free(compBuf);
		free(decompBuf);
	}
	else
	{
		if (infoStream.CopyFrom(inStream, head.infosize) != head.infosize)
			return false;
	}

	// Read file list

	uint32_t curAttrib = 0, curGroup = 0, curPass = 0;
	int64_t curOffset = head.size;
	std::string curFolder;

	infoStream.SetPos(0);
	while (infoStream.GetPos() < infoStream.GetSize())
	{
		geacompfile cf;
		geafiledesc gf;

		infoStream.Read(&cf);

		gf.flags = cf.flags;
		gf.ft = cf.ft;
		gf.size = cf.size;
		gf.compsize = cf.compsize;
		gf.crc = cf.crc;

		if (cf.flags & GEAF_ATTRIB)
		{
			infoStream.Read(&curAttrib);
		}
		gf.attrib = curAttrib;

		if (cf.flags & GEAF_VERSION)
		{
			infoStream.Read(&gf.hiver);
			infoStream.Read(&gf.lowver);
		}

		if (cf.flags & GEAF_GROUP)
		{
			infoStream.Read(&curGroup);
		}
		gf.idgroup = curGroup;

		if (cf.flags & GEAF_PROTECT)
		{
			infoStream.Read(&curPass);
		}
		gf.idpass = curPass;

		gf.name = ReadGeaString(&infoStream);

		if (cf.flags & GEAF_FOLDER)
		{
			curFolder = ReadGeaString(&infoStream);
		}
		gf.subfolder = curFolder;

		gf.offset = curOffset;
		curOffset += gf.compsize;

		m_vFiles.push_back(gf);
	}

	if (head.movedsize)
	{
		//TODO: implement
	}

	int64_t volOffs = 0;
	for (uint16_t i = 0; i < head.count; i++)
	{
		int64_t volSize;

		if (i == 0)
			volSize = head.geasize - head.movedsize - head.size;
		else if (i == head.count - 1)
			volSize = head.lastsize - sizeof(geavolume);
		else
			volSize = head.volsize - sizeof(geavolume);
			
		m_vVolOffs.push_back(volOffs);
		m_vVolSizes.push_back(volSize);

		volOffs += volSize;
	}

	m_pStream = inStream;
	m_nBlockSize = head.blocksize * 0x40000;
	m_nSolidSize = head.solidsize * 0x40000;
	m_nLastSolid = 0xFFFFFFFF;
	m_pOutBuffer = (BYTE*) malloc(m_nBlockSize + m_nSolidSize + 64);
	m_nOutBufferDataSize = 0;
	return true;
}

void GeaFile::Close()
{
	if (m_pStream)
	{
		delete m_pStream;
		m_pStream = nullptr;
	}

	if (m_pOutBuffer)
	{
		free(m_pOutBuffer);
		m_pOutBuffer = nullptr;
		m_nOutBufferDataSize = 0;
	}

	m_vFiles.clear();
	m_sPattern.clear();
	m_vPasswordCRC.clear();
	m_vPasswords.clear();
	m_vVolSizes.clear();
	m_vVolOffs.clear();
}

bool GeaFile::GetFileDesc( int index, StorageItemInfo* desc )
{
	if (index >= 0 && index < (int) m_vFiles.size())
	{
		const geafiledesc& gf = m_vFiles[index];

		std::string path = gf.name;
		if (gf.subfolder.length() > 0)
			path = gf.subfolder + "\\" + path;
		MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, desc->Path, sizeof(desc->Path) / sizeof(desc->Path[0]));
		desc->Attributes = gf.attrib;
		desc->ModificationTime = gf.ft;
		desc->Size = gf.size;
		desc->PackedSize = gf.compsize;

		if (gf.idpass > 0)
			desc->Attributes |= FILE_ATTRIBUTE_ENCRYPTED;

		return true;
	}
	return false;
}

GenteeExtractResult GeaFile::ExtractFile( int index, AStream* dest, const char* password )
{
	if (index < 0 || index >= (int)m_vFiles.size())
		return Failure;

	const geafiledesc& gf = m_vFiles[index];
	
	if ((gf.flags & GEAF_SOLID) && (index - 1 != m_nLastSolid))
	{
		GenteeExtractResult prevResult = ExtractFile(index - 1, nullptr, password);
		if (prevResult != Success)
			return prevResult;
	}

	geadata gd = {0};
	uint32_t ctype, corder;
	uint32_t fileCrc = CRC_SEED;
	std::string filePass;
	
	if (!m_pStream->SetPos(gf.offset)) return FailedRead;

	if (gf.idpass)
	{
		filePass = m_vPasswords[gf.idpass-1];
		if (filePass.length() == 0)
		{
			if (CheckPassword(gf.idpass-1, password))
				filePass = password;
			else
				return PasswordRequired;
		}
	}

	int64_t bytesLeft = gf.size;
	while (bytesLeft > 0)
	{
		slzge lz = {0};
		sppmd pm = {0};

		if (!gd.size)
		{
			uint32_t solidoff = 0;

			geadata loc_gd;
			m_pStream->Read(&loc_gd);

			gd.order = loc_gd.order & ~0x80;
			gd.size = loc_gd.size;

			//off += long( sizeof( geadata ))

			ctype = gd.order >> 4;
			corder = (gd.order & 0x0F) + 1;

			if ( ctype != GEA_LZGE || corder > 1 )
			{
				m_nOutBufferDataSize = 0;
			}
			else
			{
				// Сдвигаем данные для solid
				if (m_nOutBufferDataSize + min( bytesLeft, m_nBlockSize ) > m_nBlockSize + m_nSolidSize)
				{
					size_t delLen = m_nOutBufferDataSize - m_nSolidSize;
					memmove(m_pOutBuffer, m_pOutBuffer + delLen, m_nSolidSize);
					m_nOutBufferDataSize = m_nSolidSize;
				}
				solidoff = (uint32_t) m_nOutBufferDataSize;
			}

			lz.solidoff = solidoff;
		}
				
		uint32_t isize = min(m_nBlockSize, gd.size);
		uint32_t osize = min(m_nBlockSize, (uint32_t) bytesLeft);

		BYTE* inBuf = (BYTE*) malloc(isize);
		m_pStream->ReadBuffer(inBuf, isize);

		if (gf.idpass)
		{
			//TODO: implement
			//gea_protect(inBuf, isize, filePass.c_str());
			free(inBuf);
			return Failure;
		}

		BYTE* outBuf = nullptr;

		uint8_t order = gd.order & ~0x80;
		uint32_t ctype = order >> 4;
		uint32_t corder = ((uint32_t) order) & 0x0F + 1;

		switch (ctype)
		{
		case GEA_STORE:
			outBuf = inBuf;
			break;
		case GEA_LZGE:
			outBuf = m_pOutBuffer;
			lzge_decode(inBuf, outBuf, lz.solidoff + osize, &lz);
			m_nOutBufferDataSize = lz.solidoff + osize;
			outBuf += lz.solidoff;
			break;
		case GEA_PPMD:
			outBuf = m_pOutBuffer;
			pm.order = corder;
			ppmd_decode(inBuf, gd.size, outBuf, osize, &pm);
			break;
		default:
			free(inBuf);
			return FailedRead;
		}

		fileCrc = crc(outBuf, osize, fileCrc);

		gd.size -= isize;
		if (dest && outBuf)
		{
			if (!dest->WriteBuffer(outBuf, osize))
			{
				free(inBuf);
				return FailedWrite;
			}
		}
		bytesLeft -= osize;
		free(inBuf);
	}

	m_nLastSolid = index;

	return (bytesLeft == 0) && ((fileCrc == gf.crc) || (gf.crc == 0)) ? Success : FailedRead;
}

void GeaFile::GetFileTypeName( wchar_t* buf, size_t bufSize )
{
	wcscpy_s(buf, bufSize, L"Gentee Archive (GEA)");
}

void GeaFile::GetCompressionName( wchar_t* buf, size_t bufSize )
{
	wcscpy_s(buf, bufSize, L"LZGE");
}

std::string GeaFile::ReadGeaString( AStream* data )
{
	std::string str;
	char c;
	while(data->Read(&c) && c)
	{
		str += c;
	}
	return str;
}

bool GeaFile::CheckPassword( uint32_t passIndex, const char* password )
{
	if (!password || !*password) return false;
	if (m_vPasswordCRC.size() <= passIndex) return false;
	
	uint32_t passCrc = crc((pubyte) password, strlen(password), CRC_SEED);
	if (m_vPasswordCRC[passIndex] == passCrc)
	{
		m_vPasswords[passIndex] = password;
		return true;
	}

	return false;
}
