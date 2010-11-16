#include "stdafx.h"
#include "WiseFile.h"
#include "Unpacker.h"

#define ZIP_FILE_HEADER 0x04034b50

CWiseFile::CWiseFile()
{
	m_hSourceFile = INVALID_HANDLE_VALUE;
	m_nFilesStartPos = 0;
	m_pScriptBuf = NULL;
	m_nScriptBufSize = 0;
	m_nScriptOffsetBaseFileIndex = 0;
}

CWiseFile::~CWiseFile()
{
	if (m_hSourceFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSourceFile);
	if (m_pScriptBuf != NULL)
		free(m_pScriptBuf);
}

bool CWiseFile::Open( const wchar_t* filePath )
{
	// First check the extension
	const wchar_t* ext = wcsrchr(filePath, '.');
	if (!ext || wcscmp(ext, L".exe")) return false;
	
	// Try to open file
	m_hSourceFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (m_hSourceFile == INVALID_HANDLE_VALUE)
		return false;

	int nApproxOffset = 0, nRealOffset = 0;
	bool fIsPkzip = false;

	// Heuristic search for first file offset
	if (Approximate(nApproxOffset, fIsPkzip) && (nApproxOffset > 0))
	{
		if (FindReal(nApproxOffset, fIsPkzip, nRealOffset))
		{
			m_nFilesStartPos = nRealOffset;
			return true;
		}
	}
	
	return false;
}

bool CWiseFile::GetFileInfo( int index, WiseFileRec* infoBuf, bool &noMoreItems )
{
	noMoreItems = false;
	memset(infoBuf, 0, sizeof(WiseFileRec));
	
	int nNumFilesKnown = (int)m_vFileList.size();
	if (index < nNumFilesKnown)
	{
		*infoBuf = m_vFileList[index];
		return true;
	}

	bool fResult = true;
	for (int i = nNumFilesKnown; i <= index; i++)
	{
		int nFilePos = m_nFilesStartPos;
		if (i > 0)
			nFilePos = m_vFileList[i-1].StartOffset + m_vFileList[i-1].PackedSize + 4;

		if (SetFilePointer(m_hSourceFile, nFilePos, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			return false;

		InflatedDataInfo inflateInfo;
		bool inflRes;
		if (i == 1)
			inflRes = inflateData(m_hSourceFile, m_pScriptBuf, m_nScriptBufSize, inflateInfo);
		else
			inflRes = inflateData(m_hSourceFile, NULL, inflateInfo);
		
		if (inflRes)
		{
			unsigned long newcrc;
			DWORD dwRead;
			int attempt = 0;

			ReadFile(m_hSourceFile, &newcrc, sizeof(newcrc), &dwRead, NULL);
			while ((inflateInfo.crc != newcrc) && (attempt < 8) && (dwRead == sizeof(newcrc)))
			{
				SetFilePointer(m_hSourceFile, -3, NULL, FILE_CURRENT);
				ReadFile(m_hSourceFile, &newcrc, sizeof(newcrc), &dwRead, NULL);
				inflateInfo.packedSize += 1;  // 4 (already read crc) - 3 (back-shift)
				attempt++;
			}
			if (attempt >= 8)
				SetFilePointer(m_hSourceFile, - ((int) attempt), NULL, FILE_CURRENT);
						
			// If we have valid file entry, add it to list
			if (inflateInfo.crc == newcrc)
			{
				infoBuf->PackedSize = inflateInfo.packedSize;
				infoBuf->UnpackedSize = inflateInfo.unpackedSize;
				infoBuf->StartOffset = nFilePos;
				infoBuf->CRC32 = inflateInfo.crc;
				
				switch(i)
				{
					case 0:
						wcscpy_s(infoBuf->FileName, MAX_PATH, L"WhatIs.bin");
						break;
					case 1:
						wcscpy_s(infoBuf->FileName, MAX_PATH, L"script.bin");
						break;
					case 2:
						wcscpy_s(infoBuf->FileName, MAX_PATH, L"wise001.dll");
						break;
					default:
						TryResolveFileName(infoBuf);
						break;
				}

				if (wcscmp(infoBuf->FileName, L"%TEMP%\\%W32INST_PATH_%") == 0)
					m_nScriptOffsetBaseFileIndex = m_vFileList.size();

				m_vFileList.push_back(*infoBuf);
			}
			else
			{
				fResult = false;
				break;
			}
		}
		else
		{
			noMoreItems = true;
			fResult = false;
			break;
		}
	}
	
	return fResult;
}

// Some black magic in this function
bool CWiseFile::Approximate(int &approxOffset, bool &isPkZip)
{
	DWORD dwRead;
	BOOL res;

	BYTE *buf = (BYTE*) malloc(0xc200);
	if (buf == NULL) return false;

	SetFilePointer(m_hSourceFile, 0, NULL, FILE_BEGIN);
	res = ReadFile(m_hSourceFile, buf, 0xc000, &dwRead, NULL);
	if (!res || (dwRead != 0xc000))
	{
		free(buf);
		return false;
	}

	LONG offsa;
	LONG l0, l1, l2;
	bool pkzip;

	offsa = 0xbffc;
	l2 = 0;
	while ( ((buf[offsa] != 0) || (buf[offsa+1] != 0)) && (offsa > 0x20) && (l2 != 1) )
	{
		offsa--;
		if ( (buf[offsa] == 0) && (buf[offsa+1] == 0) )
		{
			l1 = 0;
			for (l0 = 0x01; l0 <= 0x20; l0++)
			{
				if (buf[offsa-l0] == 0) l1++;
			}
			if (l1 < 0x04) offsa -= 2;
		}
	}
	offsa += 2;
	while ( (buf[offsa+3] == 0) && (offsa+4 < 0xc000) )
		offsa += 4;
	if ( (buf[offsa] <= 0x20) && (buf[offsa+1] > 0) && (buf[offsa+1] + offsa + 3 < 0xc000) )
	{
		l1 = buf[offsa+1] + 0x02;
		l2 = 0;
		for (l0 = 0x02; l0 <= l1-0x01; l0++)
		{
			if (buf[offsa+l0] >= 0x80) l2++;
		}
		if (l2 * 0x100 / l1 < 0x10) offsa += l1;
	};
	l0 = 0x02;
	while ( (l2 != ZIP_FILE_HEADER) && (l0 < 0x80) && (offsa-l0 >= 0) && (offsa-l0 <= 0xbffc) )
	{
		memmove(&l2, &buf[offsa-l0], 4);
		l0++;
	}
	if (l0 < 0x80)
	{
		pkzip = true;
		l0 = 0x0000;
		l1 = 0x0000;

		while ( (l1 != ZIP_FILE_HEADER) && (l0 < offsa) )
		{
			memmove(&l1, &buf[l0], 4);
			l0++;
		}

		l0--;
		offsa = l0;

		if (l1 != ZIP_FILE_HEADER) pkzip = false;
	}
	else
	{
		pkzip = false;
	}

	approxOffset = offsa;
	isPkZip = pkzip;
	free(buf);

	return true;
}

bool CWiseFile::FindReal(int approxOffset, bool isPkZip, int &realOffset)
{
	DWORD dwRead;
	BOOL res;
	bool inflRes;
	unsigned long newcrc = 0;

	if (!isPkZip)
	{
		if (approxOffset < 0x100)
			approxOffset = 0x100;
		else if (approxOffset > 0xbf00)
			approxOffset = 0xbf00;
	}

	bool fValidDataFound = false;
	if ( (approxOffset >= 0) && (approxOffset <= 0xffff) )
	{
		// Detecting real archive offset
		// First in offset [0 - 0x100], then in [-0x100 - 0]
		int pos = 0;
		do
		{
			if (SetFilePointer(m_hSourceFile, approxOffset + pos, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
				return false;

			InflatedDataInfo inflateInfo;
			inflRes = inflateData(m_hSourceFile, NULL, inflateInfo);

			if (inflRes && inflateInfo.unpackedSize && inflateInfo.crc)
			{
				res = ReadFile(m_hSourceFile, &newcrc, 4, &dwRead, NULL);
				if (!res || (dwRead != 4)) return false;

				realOffset = approxOffset + pos;
				fValidDataFound = (inflateInfo.crc == newcrc);
			}
			
			pos++;
			if (pos == 0x100) pos = -0x100;

		} while ( !fValidDataFound && (pos != 0) );
	}

	return fValidDataFound;
}

bool CWiseFile::ExtractFile( int index, const wchar_t* destPath )
{
	WiseFileRec &fileInfo = m_vFileList[index];
	InflatedDataInfo inflateInfo;

	if (SetFilePointer(m_hSourceFile, fileInfo.StartOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return false;

	bool rval = inflateData(m_hSourceFile, destPath, inflateInfo);
	return rval;
}

// Suspected file opcode structure
// 1 byte - opcode - 0x00
// 2 bytes - flags (?)
// 4 bytes - offset to file start (it is unclear from where)
// 4 bytes - offset to file end (same unknown origin)
// 4 bytes - ?
// 4 bytes - uncompressed length
// 20 bytes - ?
// 4 bytes - CRC
// var - file name (0 terminated)

// Tests revealed that offset in script calculated based
// on real offset of file "%TEMP%\\%W32INST_PATH_%" (which have offset 0 in script)
// Still not sure if this is true for all cases

#pragma pack(push, 1)

struct ScriptFileRec
{
	char opCode;
	short flags;
	int startOffset;
	int endOffset;
	int date;
	int unpackedSize;
	int unknown[5];
	unsigned long fileCRC;
	char fileName[1];
};

#pragma pack(pop)

bool CWiseFile::TryResolveFileName( WiseFileRec *infoBuf )
{
	if (m_pScriptBuf == NULL || m_nScriptBufSize == 0 || infoBuf == NULL || infoBuf->UnpackedSize == 0)
		return false;

	// Search for uncompressed size number in script
	// if found check for crc after 20 bytes and fetch name after crc

	int scriptOffset = 0;
	if (m_nScriptOffsetBaseFileIndex > 0)
		scriptOffset = infoBuf->StartOffset - m_vFileList[m_nScriptOffsetBaseFileIndex].StartOffset;
	
	size_t pos = 0;
	while (pos < m_nScriptBufSize - 30)
	{
		ScriptFileRec* rec = (ScriptFileRec*) (m_pScriptBuf + pos);
		if (rec->opCode == 0 && rec->unpackedSize == infoBuf->UnpackedSize
			&& (rec->endOffset - rec->startOffset) == (infoBuf->PackedSize + 4) && (scriptOffset == 0 || scriptOffset == rec->startOffset))
		{
			if (rec->fileCRC == infoBuf->CRC32 || rec->fileCRC == 0)
			{
				MultiByteToWideChar(CP_UTF8, 0, rec->fileName, -1, infoBuf->FileName, MAX_PATH);
				FixNameDuplicates(infoBuf);

				return true;
			}
		}
		pos++;
	}

	return false;
}

static void InsertNumberInFileName(wchar_t* name, size_t nameMax, int val)
{
	wchar_t* lastDot = wcsrchr(name, '.');
	wchar_t* lastSlash = wcsrchr(name, '\\');

	wchar_t tmpBuf[MAX_PATH] = {0};
	if (lastDot != NULL && *lastDot && *(lastDot+1) && lastDot > lastSlash)
	{
		*lastDot = 0;
		swprintf_s(tmpBuf, MAX_PATH, L"%s,%d.%s", name, val, lastDot + 1);
	}
	else
	{
		swprintf_s(tmpBuf, MAX_PATH, L"%s,%d", name, val);
	}

	wcscpy_s(name, nameMax, tmpBuf);
}

void CWiseFile::FixNameDuplicates( WiseFileRec *infoBuf )
{
	size_t dupIndex;
	
	// If this is first duplicate
	if (FindRecordByName(infoBuf->FileName, &dupIndex))
	{
		int fileIndex = 2;
		int totalFiles = (int) m_vFileList.size();
		wchar_t tmpBuf[MAX_PATH];

		do
		{
			wcscpy_s(tmpBuf, MAX_PATH, infoBuf->FileName);
			InsertNumberInFileName(tmpBuf, MAX_PATH, fileIndex);
			if (!FindRecordByName(tmpBuf, &dupIndex)) break;

			fileIndex++;
		} while (fileIndex <= totalFiles);

		InsertNumberInFileName(infoBuf->FileName, MAX_PATH, fileIndex);
	}
}

bool CWiseFile::FindRecordByName( const wchar_t* fileName, size_t* foundIndex )
{
	for (size_t i = 0; i < m_vFileList.size(); i++)
	{
		WiseFileRec &nextRec = m_vFileList[i];
		if (wcscmp(nextRec.FileName, fileName) == 0)
		{
			if (foundIndex) *foundIndex = i;
			return true;
		}
	}
	return false;
}
