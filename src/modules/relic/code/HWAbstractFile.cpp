#include "stdafx.h"
#include "HWAbstractFile.h"

bool CHWAbstractStorage::GetFileInfo(int index, HWStorageItem *item) const
{
	if ((index < 0) || index >= (int) m_vItems.size())
		return false;

	*item = m_vItems[index];
	return true;
}

bool CHWAbstractStorage::ReadData( void* buf, size_t size )
{
	if (!buf) return false;
	if (!size) return true;

	DWORD dwRead;
	BOOL opRes = ReadFile(m_hInputFile, buf, size, &dwRead, NULL);
	
	return opRes && (dwRead == size);
}

bool CHWAbstractStorage::SeekPos( __int64 pos, DWORD moveMethod )
{
	LARGE_INTEGER nMoveVal, nNewPos;
	nMoveVal.QuadPart = pos;

	BOOL opRes = SetFilePointerEx(m_hInputFile, nMoveVal, &nNewPos, moveMethod);
	return (opRes == TRUE);
}
