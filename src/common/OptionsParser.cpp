#include "StdAfx.h"
#include "OptionsParser.h"

#define SECTION_BUF_SIZE 1024
#define SETTINGS_KEY_REGISTRY L"Observer"

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


string OptionsList::GetValueAsAnsiString( const wchar_t* Key, const char* Default )
{
	wstring strBuf = GetValueAsString(Key, L"");
	if (strBuf.length() > 0)
	{
		int nSize = WideCharToMultiByte(CP_ACP, 0, strBuf.c_str(), -1, NULL, 0, NULL, NULL);
		char* tmpBuf = (char*) malloc(nSize + 1);
		memset(tmpBuf, 0, nSize + 1);
		WideCharToMultiByte(CP_ACP, 0, strBuf.c_str(), -1, tmpBuf, nSize + 1, NULL, NULL);
		return tmpBuf;
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
	DWORD dwValueSize = (DWORD) OutputMaxSize;

	LSTATUS retVal = RegQueryValueEx(m_hkRegKey, ValueName, NULL, NULL, (LPBYTE) Output, &dwValueSize);
	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::GetValue( const char* ValueName, char *Output, size_t OutputMaxSize )
{
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
