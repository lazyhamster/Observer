#include "stdafx.h"
#include "RegistrySettings.h"

#define SETTINGS_KEY_REGISTRY L"Observer"

RegistrySettings::RegistrySettings( const wchar_t* RootKey )
{
	m_strRegKeyName = RootKey;
	m_strRegKeyName.append(L"\\");
	m_strRegKeyName.append(SETTINGS_KEY_REGISTRY);

	m_hkRegKey = 0;
	m_fCanWrite = false;
}

RegistrySettings::~RegistrySettings()
{
	if (m_hkRegKey != 0)
	{
		RegCloseKey(m_hkRegKey);
		m_hkRegKey = 0;
	}
}

bool RegistrySettings::Open(bool CanWrite)
{
	if (m_hkRegKey != 0)
		return true;

	LSTATUS retVal;

	if (CanWrite)
		retVal = RegCreateKey(HKEY_CURRENT_USER, m_strRegKeyName.c_str(), &m_hkRegKey);
	else
		retVal = RegOpenKey(HKEY_CURRENT_USER, m_strRegKeyName.c_str(), &m_hkRegKey);

	m_fCanWrite = CanWrite;
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
	LSTATUS retVal = RegQueryValueExA(m_hkRegKey, ValueName, NULL, NULL, (LPBYTE) Output, &dwValueSize);
	return (retVal == ERROR_SUCCESS);
}

bool RegistrySettings::GetValue( const wchar_t* ValueName, bool &Output )
{
	if (!m_hkRegKey) return false;
	
	int nValue;
	if (GetValue(ValueName, nValue))
	{
		Output = (nValue != 0);
		return true;
	}
	return false;
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

bool RegistrySettings::SetValue( const wchar_t* ValueName, bool Value )
{
	return SetValue(ValueName, Value ? 1 : 0);
}
