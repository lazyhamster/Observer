#include "StdAfx.h"
#include "OptionsParser.h"

#define SECTION_BUF_SIZE 1024

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, OptionsList &opts)
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
		
		OptionsItem opt;
		// Ignore too large options
		if ((wcslen(wszBufPtr) < OPT_KEY_MAXLEN) && (wcslen(wszSeparator + 1) < OPT_VAL_MAXLEN))
		{
			wcscpy_s(opt.key, OPT_KEY_MAXLEN, wszBufPtr);
			wcscpy_s(opt.value, OPT_VAL_MAXLEN, (wszSeparator + 1));
			opts.push_back(opt);
		}
		
		wszBufPtr += nEntryLen + 1;
	} //while
	
	return (opts.size() - nOptCountBefore > 0);
}
