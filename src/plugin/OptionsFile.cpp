#include "stdafx.h"
#include "OptionsFile.h"

#define SECTION_BUF_SIZE 1024
#define CONFIG_FILE L"observer.ini"

OptionsFile::OptionsFile( const wchar_t* ConfigLocation )
{
	swprintf_s(m_wszFilePath, MAX_PATH, L"%s%s", ConfigLocation, CONFIG_FILE);
}

OptionsList* OptionsFile::GetSection( const wchar_t* SectionName )
{
	wchar_t wszBuffer[SECTION_BUF_SIZE] = {0};
	if (GetSectionLines(SectionName, wszBuffer, SECTION_BUF_SIZE))
	{
		OptionsList *opl = new OptionsList();
		opl->ParseLines(wszBuffer);
		return opl;
	}

	return NULL;
}

bool OptionsFile::GetSectionLines( const wchar_t* SectionName, wchar_t* Buffer, size_t BufferSize )
{
	DWORD readRes = GetPrivateProfileSectionW( SectionName, Buffer, (DWORD)BufferSize, m_wszFilePath);
	return (readRes > 0) && (readRes < BufferSize - 2);
}
