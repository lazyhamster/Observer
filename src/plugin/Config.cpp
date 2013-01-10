#include "stdafx.h"
#include "Config.h"

//////////////////////////////////////////////////////////////////////////

const wchar_t* ConfigSection::GetName() const
{
	return m_SectionName.c_str();
}

StringList ConfigSection::GetKeys() const
{
	StringList keys(m_Items.size());

	for (auto cit = m_Items.cbegin(); cit != m_Items.cend(); cit++)
	{
		keys.push_back(cit->Key);
	}

	return keys;
}

bool ConfigSection::GetValueByKey( const wchar_t* Key, wstring &FoundValue ) const
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
	wstring strValue;
	if (GetValueByKey(Key, strValue))
	{
		Value = wcscmp(strValue.c_str(), L"1") == 0 || _wcsicmp(strValue.c_str(), L"true") == 0;
		return true;
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, int &Value ) const
{
	wstring strValue;
	if (GetValueByKey(Key, strValue))
	{
		Value = wcstol(strValue.c_str(), NULL, 10);
		return true;
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, wchar_t *Value, size_t MaxValueSize ) const
{
	wstring strValue;
	if (GetValueByKey(Key, strValue))
	{
		wcscpy_s(Value, MaxValueSize, strValue.c_str());
		return true;
	}

	return false;
}

bool ConfigSection::GetValue( const wchar_t* Key, char *Value, size_t MaxValueSize ) const
{
	wstring strValue;
	if (GetValueByKey(Key, strValue))
	{
		WideCharToMultiByte(CP_UTF8, 0, strValue.c_str(), -1, Value, MaxValueSize, NULL, NULL);
		return true;
	}

	return false;
}

void ConfigSection::AddItem( const wchar_t* Key, const wchar_t* Value )
{
	ConfigItem item;
	item.Key = Key;
	item.Value = Value;

	m_Items.push_back(item);
}

//////////////////////////////////////////////////////////////////////////

Config::Config()
{

}

Config::~Config()
{
	for (auto it = m_Sections.begin(); it != m_Sections.end(); it++)
	{
		delete (*it).second;
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

bool Config::ParseFile(const wstring& path)
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

		ConfigSection* pNewSection = new ConfigSection(pSectionName);
		m_Sections[pSectionName] = pNewSection;

		ParseSectionValues(pNewSection, path);

		wszSectionListBufPtr += wcslen(pSectionName);
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
				
				ConfigItem newItem;
				newItem.Key = wszEntry;
				newItem.Value = wszSeparator + 1;

				section->AddItem(wszEntry, wszSeparator + 1);
				nNumOptsFound++;
			}
			free(wszEntry);
		}
		
		dataBufferPtr += wcslen(dataBufferPtr);
	}

	return nNumOptsFound;
}
