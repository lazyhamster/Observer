#include "StdAfx.h"
#include "OptionsParser.h"

#define SECTION_BUF_SIZE 1024

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, WStringMap &opts)
{
	size_t nOptCountBefore = opts.size();
	wchar_t wszBuffer[SECTION_BUF_SIZE] = {0};

	DWORD res = GetPrivateProfileSectionW(wszSectionName, wszBuffer, SECTION_BUF_SIZE, wszConfigFile);
	if ((res == 0) || (res >= SECTION_BUF_SIZE - 2)) return false;

	wchar_t *wszBufPtr = wszBuffer;
	while (wszBufPtr && *wszBufPtr)
	{
		size_t nEntryLen = wcslen(wszBufPtr);

		// Skip leading spaces
		while (*wszBufPtr == ' ')
			wszBufPtr++;
		
		wchar_t *wszSeparator = wcschr(wszBufPtr, '=');
		if (!wszSeparator) continue;

		// Skip commented entries
		if (wszBufPtr[0] == ';') continue;

		*wszSeparator = 0;
		wstring sOpt(wszBufPtr);
		wstring sVal(wszSeparator + 1);

		opts[sOpt] = sVal;

		wszBufPtr += nEntryLen + 1;
	} //while
	
	return (opts.size() - nOptCountBefore > 0);
}
