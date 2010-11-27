#include "stdafx.h"
#include "HWAbstractFile.h"

bool CHWAbstractStorage::GetFileInfo(int index, HWStorageItem *item)
{
	if ((index < 0) || index >= (int) m_vItems.size())
		return false;

	OnGetFileInfo(index);

	*item = m_vItems[index];
	return true;
}

bool CHWAbstractStorage::ReadData( void* buf, DWORD size )
{
	DWORD nRead;
	bool opRes = ReadData(buf, size, &nRead);
	
	return opRes && (nRead == size);
}

bool CHWAbstractStorage::ReadData( void* buf, DWORD size, DWORD* numRead )
{
	if (!buf) return false;
	if (!size) return true;

	DWORD dwRead;
	BOOL opRes = ReadFile(m_hInputFile, buf, size, &dwRead, NULL);
	if (opRes)
		*numRead = dwRead;

	return (opRes == TRUE);
}

bool CHWAbstractStorage::SeekPos( __int64 pos, DWORD moveMethod )
{
	LARGE_INTEGER nMoveVal, nNewPos;
	nMoveVal.QuadPart = pos;

	BOOL opRes = SetFilePointerEx(m_hInputFile, nMoveVal, &nNewPos, moveMethod);
	return (opRes == TRUE);
}

void CHWAbstractStorage::BaseClose()
{
	if (m_bIsValid && m_hInputFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hInputFile);
		m_hInputFile = INVALID_HANDLE_VALUE;
	}

	m_vItems.clear();
	m_bIsValid = false;
}

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}
