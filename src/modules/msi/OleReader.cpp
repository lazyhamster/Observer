#include "stdafx.h"
#include "OleReader.h"

#include <objbase.h>

static const char Msi_Chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._";
static const wchar_t Msi_SpecChar = L'!';
static const unsigned Msi_NumBits = 6;
static const unsigned Msi_NumChars = 1 << Msi_NumBits;
static const unsigned Msi_CharMask = Msi_NumChars - 1;
static const unsigned Msi_StartUnicodeChar = 0x3800;
static const unsigned Msi_UnicodeRange = Msi_NumChars * (Msi_NumChars + 1);

COleStorage::COleStorage()
{
	m_pStoragePtr = nullptr;
}

COleStorage::~COleStorage()
{
	Close();
}

bool COleStorage::Open(const wchar_t* storagePath)
{
	Close();
	
	IStorage* pStor = nullptr;
	HRESULT hr;

	hr = StgOpenStorageEx(storagePath, STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE, STGFMT_STORAGE, 0, nullptr, nullptr, __uuidof(IStorage), (void**)&pStor);
	if (SUCCEEDED(hr))
	{
		IEnumSTATSTG* pEnumStat;
		hr = pStor->EnumElements(0, NULL, 0, &pEnumStat);

		if (SUCCEEDED(hr))
		{
			STATSTG rgelt = { 0 };
			ULONG celtFetched = 0;
			int counter = 1;
			while (pEnumStat->Next(1, &rgelt, &celtFetched) == S_OK)
			{
				// We need only streams in storage
				if (rgelt.type == STGTY_STREAM)
				{
					std::wstring fileName;
					if (CompoundMsiNameToFileName(rgelt.pwcsName, fileName))
						m_mStreamNames[fileName] = rgelt.pwcsName;
				}
				
				CoTaskMemFree(rgelt.pwcsName);
			}
			pEnumStat->Release();

			m_pStoragePtr = pStor;

			return true;
		}

		pStor->Release();
	}
	
	return false;
}

void COleStorage::Close()
{
	if (m_pStoragePtr)
	{
		((IStorage*)m_pStoragePtr)->Release();
		m_pStoragePtr = nullptr;
	}

	m_mStreamNames.clear();
}

bool COleStorage::ExtractStream(const wchar_t* streamFileName, const wchar_t* destPath)
{
	auto findIt = m_mStreamNames.find(streamFileName);
	if (findIt == m_mStreamNames.end())
		return false;

	auto streamCompoundName = findIt->second;
	IStorage* pStor = (IStorage*)m_pStoragePtr;

	IStream *pStream;
	HRESULT hr = pStor->OpenStream(streamCompoundName.c_str(), NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
	if (SUCCEEDED(hr))
	{
		STATSTG stat;
		if (SUCCEEDED(pStream->Stat(&stat, STATFLAG_DEFAULT)))
		{
			HANDLE hOutFile = CreateFile(destPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if (hOutFile == INVALID_HANDLE_VALUE)
			{
				pStream->Release();
				return false;
			}
			
			const ULONG cnCopyBufSize = 64 * 1024;
			char copyBuf[cnCopyBufSize];

			ULONG copySize;
			ULONG cbRead;

			bool isSuccess = true;
			ULONGLONG bytesLeft = stat.cbSize.QuadPart;
			while (bytesLeft > 0)
			{
				copySize = (ULONG) min(cnCopyBufSize, bytesLeft);
				hr = pStream->Read(copyBuf, copySize, &cbRead);
				if (SUCCEEDED(hr))
				{
					WriteFile(hOutFile, copyBuf, copySize, &cbRead, NULL);
					bytesLeft -= copySize;
				}
				else
				{
					isSuccess = false;
					break;
				}
			}

			CloseHandle(hOutFile);
			pStream->Release();

			return isSuccess;
		}
	}

	return false;
}

// Code from 7zip sources
bool COleStorage::CompoundMsiNameToFileName(const wchar_t *compoundMsiName, std::wstring &outFileName)
{
	size_t name_len = wcslen(compoundMsiName);

	for (size_t i = 0; i < name_len; i++)
	{
		wchar_t c = compoundMsiName[i];
		if (c < Msi_StartUnicodeChar || c > Msi_StartUnicodeChar + Msi_UnicodeRange)
			return false;

		c -= Msi_StartUnicodeChar;

		unsigned c0 = (unsigned)c & Msi_CharMask;
		unsigned c1 = (unsigned)c >> Msi_NumBits;

		if (c1 <= Msi_NumChars)
		{
			outFileName += (wchar_t)Msi_Chars[c0];
			if (c1 == Msi_NumChars)
				break;
			outFileName += (wchar_t)Msi_Chars[c1];
		}
		else
			outFileName += Msi_SpecChar;
	}
	return true;
}
