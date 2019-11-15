#ifndef ExtFilter_h__
#define ExtFilter_h__

struct ExtensionsFilter
{
	ExtensionsFilter() { m_fEmptyIsAny = false; }
	ExtensionsFilter(bool emptyIsAny) { m_fEmptyIsAny = emptyIsAny; }

	int SetFilter(const std::wstring& filterStr);
	void Clear();

	bool DoesPathMatch(const wchar_t* path) const;

private:
	std::vector<std::wstring> m_vExtensionFilter;
	bool m_fEmptyIsAny;  // If true then empty filter means match all, if false then empty filter matches nothing
};

#endif // ExtFilter_h__
