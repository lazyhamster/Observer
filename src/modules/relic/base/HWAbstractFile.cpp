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

void CHWAbstractStorage::BaseClose()
{
	if (m_pInputFile != NULL)
	{
		delete m_pInputFile;
		m_pInputFile = NULL;
	}

	m_vItems.clear();
	m_bIsValid = false;
}
