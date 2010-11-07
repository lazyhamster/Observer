#include "stdafx.h"
#include "WiseFile.h"
#include "Unpacker.h"

#define ZIP_FILE_HEADER 0x04034b50

CWiseFile::CWiseFile()
{
	m_hSourceFile = INVALID_HANDLE_VALUE;
	m_nFilesStartPos = 0;
}

CWiseFile::~CWiseFile()
{
	if (m_hSourceFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hSourceFile);
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

		unsigned int packedSize, unpackedSize;
		unsigned long inflateCrc;
		bool inflRes = inflateData(m_hSourceFile, INVALID_HANDLE_VALUE, packedSize, unpackedSize, inflateCrc);
		
		if (inflRes)
		{
			unsigned long newcrc;
			DWORD dwRead;
			int attempt = 0;

			ReadFile(m_hSourceFile, &newcrc, sizeof(newcrc), &dwRead, NULL);
			while ((inflateCrc != newcrc) && (attempt < 8) && (dwRead == sizeof(newcrc)))
			{
				SetFilePointer(m_hSourceFile, -3, NULL, FILE_CURRENT);
				ReadFile(m_hSourceFile, &newcrc, sizeof(newcrc), &dwRead, NULL);
				attempt++;
			}
			if (attempt >= 8)
				SetFilePointer(m_hSourceFile, - ((int) attempt), NULL, FILE_CURRENT);
						
			// If we have valid file entry, add it to list
			if (inflateCrc == newcrc)
			{
				infoBuf->PackedSize = packedSize;
				infoBuf->UnpackedSize = unpackedSize;
				infoBuf->StartOffset = nFilePos;
				infoBuf->CRC32 = inflateCrc;
				
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
				}

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
	unsigned long inflateCrc;
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

			unsigned int packedSize, unpackedSize;
			inflRes = inflateData(m_hSourceFile, INVALID_HANDLE_VALUE, packedSize, unpackedSize, inflateCrc);

			if (inflRes && unpackedSize && inflateCrc)
			{
				res = ReadFile(m_hSourceFile, &newcrc, 4, &dwRead, NULL);
				if (!res || (dwRead != 4)) return false;

				realOffset = approxOffset + pos;
				fValidDataFound = (inflateCrc == newcrc);
			}
			
			pos++;
			if (pos == 0x100) pos = -0x100;

		} while ( !fValidDataFound && (pos != 0) );
	}

	return fValidDataFound;
}
