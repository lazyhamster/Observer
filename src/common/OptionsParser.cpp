#include "StdAfx.h"
#include "OptionsParser.h"
#include <errno.h>

#define SECTION_BUF_SIZE 1024
#define CONFIG_FILE L"observer.ini"

//////////////////////////////////////////////////////////////////////////

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

bool OptionsList::GetValue( const wchar_t* Key, bool &Value ) const
{
	wchar_t tmpBuf[10] = {0};
	if ( GetValue(Key, tmpBuf, ARRAY_SIZE(tmpBuf)) )
	{
		Value = _wcsicmp(tmpBuf, L"true") || wcscmp(tmpBuf, L"1");
		return true;
	}

	return false;
}

bool OptionsList::GetValue( const wchar_t* Key, int &Value ) const
{
	wchar_t tmpBuf[10] = {0};
	if ( GetValue(Key, tmpBuf, ARRAY_SIZE(tmpBuf)) )
	{
		int resVal = wcstol(tmpBuf, NULL, 10);
		
		if (resVal != 0 || errno != EINVAL)
		{
			Value = resVal;
			return true;
		}
	}

	return false;
}

bool OptionsList::GetValue( const wchar_t* Key, wchar_t *Value, size_t MaxValueSize ) const
{
	vector<OptionsItem>::const_iterator cit;
	for (cit = m_vValues.begin(); cit != m_vValues.end(); cit++)
	{
		if (_wcsicmp(cit->key, Key) == 0)
		{
			if (wcslen(cit->value) >= MaxValueSize) break;

			wcscpy_s(Value, MaxValueSize, cit->value);
			return true;
		}
	} // for

	return false;
}

bool OptionsList::GetValue( const wchar_t* Key, char *Value, size_t MaxValueSize ) const
{
	wchar_t* tmpBuf = (wchar_t*) malloc(MaxValueSize * sizeof(wchar_t));
	memset(tmpBuf, 0, MaxValueSize * sizeof(wchar_t));

	if ( GetValue(Key, tmpBuf, MaxValueSize) )
	{
		WideCharToMultiByte(CP_ACP, 0, tmpBuf, -1, Value, (int)MaxValueSize, NULL, NULL);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

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
