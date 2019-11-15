#include "stdafx.h"
#include "ExtFilter.h"

bool ExtensionsFilter::DoesPathMatch(const wchar_t* path) const
{
	if (m_vExtensionFilter.empty())
		return m_fEmptyIsAny;

	for (size_t i = 0; i < m_vExtensionFilter.size(); ++i)
	{
		if (PathMatchSpec(path, m_vExtensionFilter[i].c_str()))
			return true;
	}

	return false;
}

int ExtensionsFilter::SetFilter(const std::wstring& filterStr)
{
	if (filterStr.empty()) return 0;

	wchar_t* context = NULL;
	wchar_t* strList = _wcsdup(filterStr.c_str());
	int numAdded = 0;

	wchar_t* token = wcstok_s(strList, L";", &context);
	while (token)
	{
		if (wcslen(token) > 0)
		{
			while (!token[0]) token++;
			m_vExtensionFilter.push_back(token);
			numAdded++;
		}

		token = wcstok_s(NULL, L";", &context);
	}
	free(strList);

	return numAdded;
}

void ExtensionsFilter::Clear()
{
	m_vExtensionFilter.clear();
}
