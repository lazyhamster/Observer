#include "stdafx.h"
#include "MailReader.h"

std::wstring ConvertString(const char* src)
{
	if (!src || !*src) return L"";
	
	int tmpBufSize = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
	if (tmpBufSize == 0) return L"";
	
	wchar_t* tmpBuf = (wchar_t*) malloc(tmpBufSize * sizeof(wchar_t));
	memset(tmpBuf, 0, tmpBufSize * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, src, -1, tmpBuf, (int) tmpBufSize);

	std::wstring retVal(tmpBuf);
	free(tmpBuf);

	return retVal;
}

static bool IsBadPathChar(wchar_t c)
{
	return wcschr(L"\r\n", c) != nullptr;
}

void SanitizeString(std::wstring &str)
{
	wchar_t bad_chars[] = L"\r\n";
	
	str.erase(std::remove_if(str.begin(), str.end(), IsBadPathChar), str.end());
}

//////////////////////////////////////////////////////////////////////////

bool IMailReader::Open( const wchar_t* filepath )
{
	Close();
	
	FILE* fp = NULL;
	errno_t err = _wfopen_s(&fp, filepath, L"rb");

	if ((err == 0) && IsValidFile(fp))
	{
		m_pSrcFile = fp;
		return true;
	}
	else
	{
		if (fp) fclose(fp);
		return false;
	}
}

void IMailReader::Close()
{
	m_vItems.clear();

	if (m_pSrcFile != NULL)
	{
		fclose(m_pSrcFile);
		m_pSrcFile = NULL;
	}
}

int IMailReader::Extract( int itemindex, const wchar_t* destpath )
{
	if (m_pSrcFile == NULL)
		return SER_ERROR_SYSTEM;
	
	FILE* outp;
	if (_wfopen_s(&outp, destpath, L"wb") != 0)
		return SER_ERROR_WRITE;

	const MBoxItem& item = m_vItems[itemindex];

	size_t copySize = (size_t) item.Size();
	char* copyBuf = (char*) malloc(copySize);
	int nRet = SER_SUCCESS;
	
	_fseeki64(m_pSrcFile, item.StartPos, SEEK_SET);
	if (fread(copyBuf, 1, copySize, m_pSrcFile) != copySize)
		nRet = SER_ERROR_READ;
	else if (fwrite(copyBuf, 1, copySize, outp) != copySize)
		nRet = SER_ERROR_WRITE;
	
	fclose(outp);
	free(copyBuf);

	if (nRet != SER_SUCCESS)
		_wunlink(destpath);

	return nRet;
}
