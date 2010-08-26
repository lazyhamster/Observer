#include "StdAfx.h"
#include "OptionsParser.h"

#define SECTION_BUF_SIZE 1024

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, OptionsList &opts)
{
	wchar_t wszBuffer[SECTION_BUF_SIZE] = {0};

	DWORD res = GetPrivateProfileSectionW(wszSectionName, wszBuffer, SECTION_BUF_SIZE, wszConfigFile);
	if ((res == 0) || (res >= SECTION_BUF_SIZE - 2)) return false;

	int nNumOpts = opts.ParseLines(wszBuffer);
	return (nNumOpts > 0);
}

int OptionsList::ParseLines( const wchar_t* Input )
{
	int nNumOptsFound = 0;
	
	const wchar_t *wszBufPtr = Input;
	while (wszBufPtr && *wszBufPtr)
	{
		size_t nEntryLen = wcslen(wszBufPtr);

		// Skip leading spaces and/or tabs
		while (*wszBufPtr == ' ' || *wszBufPtr == '\t')
			wszBufPtr++;

		// Skip commented entries
		if (wszBufPtr[0] != ';')
		{
			wchar_t *wszEntry = _wcsdup(wszBufPtr);
			wchar_t *wszSeparator = wcschr(wszEntry, '=');
			if (wszSeparator)
			{
				*wszSeparator = 0;
				AddOption(wszEntry, wszSeparator + 1);
				nNumOptsFound++;
			}
			free(wszEntry);
		}

		wszBufPtr += nEntryLen + 1;
	} //while

	return nNumOptsFound;
}

bool OptionsList::AddOption( const wchar_t* Key, const wchar_t* Value )
{
	// Ignore too large options
	if ((wcslen(Key) < OPT_KEY_MAXLEN) && (wcslen(Value) < OPT_VAL_MAXLEN))
	{
		OptionsItem opt;
		wcscpy_s(opt.key, OPT_KEY_MAXLEN, Key);
		wcscpy_s(opt.value, OPT_VAL_MAXLEN, Value);
		m_vValues.push_back(opt);

		return true;
	}
	
	return false;
}

wstring OptionsList::GetValueAsString( const wchar_t* Key, const wchar_t* Default )
{
	vector<OptionsItem>::const_iterator cit;
	for (cit = m_vValues.begin(); cit != m_vValues.end(); cit++)
	{
		if (_wcsicmp(cit->key, Key) == 0)
			return cit->value;
	}
	return Default;
}

int OptionsList::GetValueAsInt( const wchar_t* Key, int Default )
{
	wstring StrVal = GetValueAsString(Key, L"");
	if (StrVal.length() > 0)
	{
		int retVal= _wtoi(StrVal.c_str());
		if ((retVal != 0) || !errno)
			return retVal;
	}

	return Default;
}

bool OptionsList::GetValueAsBool( const wchar_t* Key, bool Default )
{
	wstring StrVal = GetValueAsString(Key, L"");
	if (StrVal.length() > 0)
		return (_wcsicmp(StrVal.c_str(), L"true") == 0) || (_wtoi(StrVal.c_str()) != 0);
	
	return Default;
}
