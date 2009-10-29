#pragma once

struct CabCacheItem;

class CCabControl
{
private:
	map<wstring, CabCacheItem*> m_mCabCache;

	CabCacheItem* getCacheItem(const wchar_t* cabName, const wchar_t* cabPath);

public:
	CCabControl(void);
	~CCabControl(void);

	int ExtractFile(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, const wchar_t* destFilePath);
	bool GetFileAttributes(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, WIN32_FIND_DATAW &fd);
};
