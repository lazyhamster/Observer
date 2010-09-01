#include "StdAfx.h"
#include "OptionsParser.h"
#include <errno.h>

#define SECTION_BUF_SIZE 1024
#define SETTINGS_KEY_REGISTRY L"Observer"
#define CONFIG_FILE L"observer.ini"

static wstring ConvertString(const char* Input)
{
	int nNumWChars = MultiByteToWideChar(CP_ACP, 0, Input, -1, NULL, NULL);
	wchar_t* buf = new wchar_t[nNumWChars + 1];
	MultiByteToWideChar(CP_ACP, 0, Input, -1, buf, nNumWChars + 1);
	wstring strResult(buf);
	delete [] buf;

	return strResult;
}

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
		WideCharToMultiByte(CP_ACP, 0, tmpBuf, -1, Value, MaxValueSize, NULL, NULL);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

RegistrySettings::RegistrySettings( const wchar_t* RootKey )
{
	m_strRegKeyName = RootKey;
	m_strRegKeyName.append(L"\\");
	m_strRegKeyName.append(SETTINGS_KEY_REGISTRY);

	m_hkRegKey = 0;
}

RegistrySettings::RegistrySettings( const char* RootKey )
{
	m_strRegKeyName = ConvertString(RootKey);
	m_strRegKeyName.append(L"\\");
	m_strRegKeyName.append(SETTINGS_KEY_REGISTRY);

	m_hkRegKey = 0;
}

RegistrySettings::~RegistrySettings()
{
	if (m_hkRegKey != 0)
	{
		RegCloseKey(m_hkRegKey);
		m_hkRegKey = 0;
	}
}

bool RegistrySettings::Open(int CanWrite)
{
	if (m_hkRegKey != 0)
		return true;
	
	LSTATUS retVal;
	
	if (CanWrite)
		retVal = RegCreateKey(HKEY_CURRENT_USER, m_strRegKeyName.c_str(), &m_hkRegKey);
	else
		retVal = RegOpenKey(HKEY_CURRENT_USER, m_strRegKeyName.c_str(), &m_hkRegKey);

	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::GetValue( const wchar_t* ValueName, int &Output )
{
	if (!m_hkRegKey) return false;

	int Value;
	DWORD dwValueSize = sizeof(Value);

	LSTATUS retVal = RegQueryValueEx(m_hkRegKey, ValueName, NULL, NULL, (LPBYTE) &Value, &dwValueSize);
	if (retVal == ERROR_SUCCESS)
	{
		Output = Value;
		return true;
	}

	return false;
}

bool RegistrySettings::GetValue( const wchar_t* ValueName, wchar_t *Output, size_t OutputMaxSize )
{
	if (!m_hkRegKey) return false;
	
	DWORD dwValueSize = (DWORD) OutputMaxSize;
	LSTATUS retVal = RegQueryValueEx(m_hkRegKey, ValueName, NULL, NULL, (LPBYTE) Output, &dwValueSize);
	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::GetValue( const char* ValueName, char *Output, size_t OutputMaxSize )
{
	if (!m_hkRegKey) return false;

	DWORD dwValueSize = (DWORD) OutputMaxSize;
	LSTATUS retVal = RegQueryValueExA(m_hkRegKey, ValueName, NULL, NULL, (LPBYTE) &Output, &dwValueSize);
	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::SetValue( const wchar_t* ValueName, int Value )
{
	if (!m_fCanWrite || !m_hkRegKey) return false;

	LSTATUS retVal = RegSetValueEx(m_hkRegKey, ValueName, 0, REG_DWORD, (LPBYTE) &Value, sizeof(Value));
	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::SetValue( const wchar_t* ValueName, const wchar_t *Value )
{
	if (!m_fCanWrite || !m_hkRegKey) return false;

	size_t nDataSize = (wcslen(Value) + 1) * sizeof(wchar_t);
	LSTATUS retVal = RegSetValueEx(m_hkRegKey, ValueName, 0, REG_SZ, (LPBYTE) Value, (DWORD) nDataSize);
	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::SetValue( const char* ValueName, const char *Value )
{
	if (!m_fCanWrite || !m_hkRegKey) return false;

	size_t nDataSize = (strlen(Value) + 1) * sizeof(char);
	LSTATUS retVal = RegSetValueExA(m_hkRegKey, ValueName, 0, REG_SZ, (LPBYTE) Value, (DWORD) nDataSize);
	return (retVal == ERROR_SUCCESS);
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
	DWORD readRes = GetPrivateProfileSectionW( SectionName, Buffer, BufferSize, m_wszFilePath);
	return (readRes > 0) && (readRes < BufferSize - 2);
}
