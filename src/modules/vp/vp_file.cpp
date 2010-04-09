#include "StdAfx.h"
#include "vp_file.h"

static void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

const wchar_t* GetFileName(const wchar_t* fullPath)
{
	const wchar_t* lastSlash = wcsrchr(fullPath, '\\');
	if (lastSlash)
		return lastSlash + 1;
	else
		return fullPath;
}

//////////////////////////////////////////////////////////////////////////

CVPFile::CVPFile()
{
	m_hFile = INVALID_HANDLE_VALUE;
}

CVPFile::~CVPFile()
{
	Close();
}

bool CVPFile::Open( const wchar_t* path )
{
	VP_Header header;
	DWORD dwReadBytes;
	LARGE_INTEGER nFileSize = {0};

	Close();
	
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (!ReadFile(hFile, &header, sizeof(header), &dwReadBytes, NULL) || (dwReadBytes != sizeof(header)))
	{
		goto err_exit;
	}

	// Check header
	if ((strncmp(header.header, VP_HEADER_MAGIC, 4) != 0) || (header.version > 2) || !header.direntries)
	{
		goto err_exit;
	}

	if (!GetFileSizeEx(hFile, &nFileSize) || (nFileSize.HighPart > 0)
		|| (nFileSize.QuadPart < header.diroffset + (__int64) header.direntries * sizeof(VP_DirEntry)))
	{
		goto err_exit;
	}

	// Go to index start
	if (SetFilePointer(hFile, header.diroffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		goto err_exit;
	}

	wchar_t wszItemName[MAX_PATH] = {0};
	wchar_t wszCurrentDir[MAX_PATH] = {0};
	size_t nCurrentDirLen = 0;

	m_vContent.reserve(header.direntries);
	for (int i = 0; i < header.direntries; i++)
	{
		VP_DirEntry dirent;
		VP_FileRec frec = {0};

		if (!ReadFile(hFile, &dirent, sizeof(dirent), &dwReadBytes, NULL) || (dwReadBytes != sizeof(dirent)))
		{
			goto err_exit;
		}

		if (strcmp(dirent.name, "..") == 0)
		{
			wchar_t* lastSlash = wcsrchr(wszCurrentDir, '\\');
			if (lastSlash)
				*lastSlash = 0;
			else
				wszCurrentDir[0] = 0;
		}
		else
		{
			frec.offset = dirent.offset;
			frec.size = dirent.size;
			
			if (dirent.timestamp > 0)
				UnixTimeToFileTime(dirent.timestamp, &frec.timestamp);

			wmemset(wszItemName, 0, MAX_PATH);
			MultiByteToWideChar(CP_UTF8, 0, dirent.name, (int) strlen(dirent.name) + 1, wszItemName, MAX_PATH);

			if (nCurrentDirLen > 0)
			{
				wcscpy_s(frec.full_path, MAX_PATH, wszCurrentDir);
				wcscat_s(frec.full_path, MAX_PATH, L"\\");
			}
			wcscat_s(frec.full_path, MAX_PATH, wszItemName);
			
			m_vContent.push_back(frec);
			
			// Save current directory path
			if (frec.IsDir())
			{
				wcscpy_s(wszCurrentDir, MAX_PATH, frec.full_path);
				nCurrentDirLen = wcslen(wszCurrentDir);
			}
		}
	}

	m_hFile = hFile;
	return true;

err_exit:
	CloseHandle(hFile);
	m_vContent.clear();
	return false;
}

void CVPFile::Close()
{
	m_vContent.clear();

	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

bool CVPFile::GetItem( int index, VP_FileRec &frec )
{
	if (index < 0 || index >= (int) m_vContent.size())
		return false;

	VP_FileRec &item = m_vContent[index];
	memcpy(&frec, &item, sizeof(VP_FileRec));
	return true;
}

int CVPFile::ExtractSingleFile( int index, const wchar_t* destPath, const ExtractProcessCallbacks* epc )
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return SER_ERROR_SYSTEM;
	
	VP_FileRec &frec = m_vContent[index];
	if (frec.IsDir()) return SER_ERROR_SYSTEM;

	if (SetFilePointer(m_hFile, frec.offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return SER_ERROR_READ;

	const wchar_t* fileName = GetFileName(frec.full_path);
	size_t nFullPathLen = wcslen(destPath) + wcslen(fileName) + 1;
	wchar_t* fullPath = (wchar_t*) malloc(nFullPathLen * sizeof(wchar_t));
	swprintf_s(fullPath, nFullPathLen, L"%s%s", destPath, fileName);
	
	HANDLE hOutput = CreateFile(fullPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	
	free(fullPath);
	if (hOutput == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	int ret = SER_SUCCESS;

	int bufSize = 16 * 1024;
	char* buf = (char *) malloc(bufSize);

	DWORD dwReadCount, dwWriteCount;
	int nBytesLeft = frec.size;
	while (nBytesLeft > 0)
	{
		DWORD dwNumRead = (nBytesLeft > bufSize) ? bufSize : nBytesLeft;
		
		if (!ReadFile(m_hFile, buf, dwNumRead, &dwReadCount, NULL) || (dwReadCount != dwNumRead))
		{
			ret = SER_ERROR_READ;
			break;
		}
		
		if (!WriteFile(hOutput, buf, dwReadCount, &dwWriteCount, NULL) || (dwWriteCount != dwReadCount))
		{
			ret = SER_ERROR_WRITE;
			break;
		}

		nBytesLeft -= dwNumRead;
	}
	
	free(buf);
	CloseHandle(hOutput);
	return ret;
}
