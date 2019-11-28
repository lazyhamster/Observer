#include "stdafx.h"
#include "Config.h"

//////////////////////////////////////////////////////////////////////////

const wchar_t* ConfigSection::GetName() const
{
	return m_SectionName.c_str();
}

bool ConfigSection::GetValueByKey( const wchar_t* Key, std::wstring &FoundValue ) const
{
	for (auto cit = m_Items.cbegin(); cit != m_Items.cend(); cit++)
	{
		if (wcscmp(cit->Key.c_str(), Key) == 0)
		{
			FoundValue = cit->Value;
			return true;
		}
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, bool &Value ) const
{
	std::wstring strValue;
	if (GetValueByKey(Key, strValue))
	{
		Value = wcscmp(strValue.c_str(), L"1") == 0 || _wcsicmp(strValue.c_str(), L"true") == 0;
		return true;
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, int &Value ) const
{
	std::wstring strValue;
	if (GetValueByKey(Key, strValue))
	{
		Value = wcstol(strValue.c_str(), NULL, 10);
		return true;
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, wchar_t *Value, size_t MaxValueSize ) const
{
	std::wstring strValue;
	if (GetValueByKey(Key, strValue) && (strValue.length() < MaxValueSize))
	{
		wcscpy_s(Value, MaxValueSize, strValue.c_str());
		return true;
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, std::wstring &Value ) const
{
	return GetValueByKey(Key, Value);
}

bool ConfigSection::GetValue( const wchar_t* Key, wchar_t &Value ) const
{
	std::wstring strValue;
	if (GetValueByKey(Key, strValue) && (strValue.length() >= 1))
	{
		Value = strValue[0];
		return true;
	}

	return false;
}

void ConfigSection::AddItem( const wchar_t* Key, const wchar_t* Value )
{
	// Try to find and replace existing value
	for (auto it = m_Items.begin(); it != m_Items.end(); it++)
	{
		ConfigItem& item = *it;
		if (wcscmp(item.Key.c_str(), Key) == 0)
		{
			item.Value = Value;
			return;
		}
	}

	// If it is new value - add it

	ConfigItem item;
	item.Key = Key;
	item.Value = Value;

	m_Items.push_back(item);
}

std::wstring ConfigSection::GetAll() const
{
	std::wstring val;
	for (size_t i = 0; i < m_Items.size(); i++)
	{
		const ConfigItem& item = m_Items[i];
		val += item.ToString();
		val += L'\0';
	}
	val += L'\0';

	return val;
}

//////////////////////////////////////////////////////////////////////////

Config::Config()
{

}

Config::~Config()
{
	for (auto it = m_Sections.begin(); it != m_Sections.end(); it++)
	{
		ConfigSection* pSection = (*it).second;
		delete pSection;
	}
	m_Sections.clear();
}

ConfigSection* Config::GetSection( const wchar_t* sectionName )
{
	auto it = m_Sections.find(sectionName);
	if (it == m_Sections.end())
		return NULL;
	else
		return (*it).second;
}

bool Config::IsSectionExists( const wchar_t* sectionName )
{
	return (GetSection(sectionName) != NULL);
}

ConfigSection* Config::AddSection( const wchar_t* sectionName )
{
	ConfigSection* pSection = GetSection(sectionName);
	
	if (pSection == NULL)
	{
		pSection = new ConfigSection(sectionName);
		m_Sections[sectionName] = pSection;
	}

	return pSection;
}

bool Config::ParseFile(const std::wstring& path)
{
	return ParseFile(path.c_str());
}

bool Config::ParseFile( const wchar_t* path )
{
	const size_t cnSectionListBufSize = 1024;
	wchar_t wszSectionListBuf[cnSectionListBufSize];

	DWORD numCharsRead = GetPrivateProfileSectionNames(wszSectionListBuf, cnSectionListBufSize, path);
	
	// Check if buffer was too small
	if (numCharsRead == cnSectionListBufSize - 2) return false;

	wchar_t* wszSectionListBufPtr = wszSectionListBuf;
	while (wszSectionListBufPtr && *wszSectionListBufPtr)
	{
		const wchar_t* pSectionName = wszSectionListBufPtr;

		ConfigSection* pSection = AddSection(pSectionName);
		ParseSectionValues(pSection, path);

		wszSectionListBufPtr += wcslen(pSectionName) + 1;
	}
	
	return true;
}

int Config::ParseSectionValues( ConfigSection* section, const wchar_t* configFilePath )
{
	const size_t dataBufferSize = 4 * 1024;
	wchar_t dataBuffer[dataBufferSize];

	DWORD numBytesRead = GetPrivateProfileSection(section->GetName(), dataBuffer, dataBufferSize, configFilePath);
	if ((numBytesRead == 0) || (numBytesRead >= dataBufferSize - 2)) return - 1;

	int nNumOptsFound = 0;
	wchar_t* dataBufferPtr = dataBuffer;

	// Cycle through entries
	while (dataBufferPtr && *dataBufferPtr)
	{
		// Skip leading space characters
		while (*dataBufferPtr == ' ' || *dataBufferPtr == '\t')
			dataBufferPtr++;

		// Skip comments
		if (dataBufferPtr[0] != ';')
		{
			wchar_t *wszEntry = _wcsdup(dataBufferPtr);
			wchar_t *wszSeparator = wcschr(wszEntry, '=');
			if (wszSeparator)
			{
				*wszSeparator = 0;
				section->AddItem(wszEntry, wszSeparator + 1);
				nNumOptsFound++;
			}
			free(wszEntry);
		}
		
		dataBufferPtr += wcslen(dataBufferPtr) + 1;
	}

	return nNumOptsFound;
}
