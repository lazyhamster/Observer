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

void SanitizeString(std::wstring &str)
{
	static wchar_t bad_chars[] = L"\r\n\t";

	// Trim left
	auto pos = str.find_first_not_of(bad_chars);
	if (pos > 0)
		str.erase(0, pos);
	
	// Trim right
	auto pos_back = str.find_last_not_of(bad_chars);
	if ((pos_back != std::wstring::npos) && (pos_back < str.length()))
		str.erase(pos_back + 1);
	
	// Replace symbols in the middle with spaces
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (wcschr(L"\r\n", str[i]) != nullptr)
			str[i] = ' ';
	}
}

time_t ConvertGDateTimeToUnixUtc(GDateTime* gdt)
{
	GDateTime* dtUtc = g_date_time_to_utc(gdt);
	time_t unixUtc = g_date_time_to_unix(dtUtc);
	g_date_time_unref(dtUtc);

	return unixUtc;
}

//////////////////////////////////////////////////////////////////////////

bool IMailReader::Open( const wchar_t* filepath )
{
	Close();
	
	FILE* fp = NULL;
	errno_t err = _wfopen_s(&fp, filepath, L"rb");

	if (err != 0) return false;

	char* sampleBuf[100] = {0};
	size_t sampleSize = _countof(sampleBuf);

	if ((fread(sampleBuf, 1, sampleSize, fp) != sampleSize) || !CheckSample(sampleBuf, sampleSize))
	{
		if (fp) fclose(fp);
		return false;
	}

	m_pSrcFile = fp;
	fseek(fp, 0, SEEK_SET);
	
	return true;
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

	size_t copySize = (size_t) item.GetSize();
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

const char* IMailReader::GetSenderAddress(GMimeMessage* message)
{
	InternetAddressList* senderList = g_mime_message_get_from(message);
	if (internet_address_list_length(senderList) > 0)
	{
		InternetAddress* addr0 = internet_address_list_get_address(senderList, 0);
		if (INTERNET_ADDRESS_IS_MAILBOX(addr0))
		{
			InternetAddressMailbox* mailBoxAddr = (InternetAddressMailbox*)addr0;
			return internet_address_mailbox_get_addr(mailBoxAddr);
		}
		else
		{
			return internet_address_get_name(addr0);
		}
	}

	return nullptr;
}
