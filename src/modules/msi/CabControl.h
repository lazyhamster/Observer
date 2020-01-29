#pragma once

#include "CabSystem.h"

struct CabCacheItem;

class CCabControl
{
private:
	std::map<std::wstring, CabCacheItem*> m_mCabCache;
	MSIHANDLE m_hOwner;

	CabCacheItem* getCacheItem(const wchar_t* cabName, const wchar_t* cabPath);

public:
	CCabControl(void);
	~CCabControl(void);

	bool ExtractFile(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, const wchar_t* destFilePath);
	bool GetFileAttributes(const wchar_t* cabName, const wchar_t* cabPath, const wchar_t* sourceFileName, WIN32_FIND_DATAW &fd);

	void SetOwner(MSIHANDLE owner) { m_hOwner = owner; }
};
