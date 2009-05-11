#ifndef _NSIS_ARCHIVE_H_
#define _NSIS_ARCHIVE_H_

#include "..\ModuleDef.h"

#include "7zip/Common/FileStreams.h"
#include "7zip/Archive/Nsis/NsisIn.h"

using namespace NArchive::NNsis;

class CNsisArchive
{
private:
	UString m_archiveName;
	CInFileStream* m_stream;
	CInArchive* m_inArc;

	int m_numFiles;
	int m_numDirectories;
	__int64 m_totalSize;

	UStringVector m_folderList;  // Fake folders list, since format does not expose folders

	bool GetUncompressedSize(const CItem &item, DWORD &size);
	int findItemIndex(const wchar_t* path);
	UString getItemPath(CItem &item);

public:
	CNsisArchive();
	~CNsisArchive();

	int Open(const wchar_t* path);
	void Close();

	int GetTotalFiles() { return m_numFiles; };
	int GetTotalDirectories() { return m_numDirectories; };
	__int64 GetTotalSize() { return m_totalSize; };

	int GetItemsCount();
	int GetItem(int itemIndex, WIN32_FIND_DATAW *itemData, wchar_t* itemPath, size_t itemPathSize);

	int ExtractItemByName(const wchar_t* itemName, const wchar_t* destDir, const ExtractProcessCallbacks* epc);
};

#endif //_NSIS_ARCHIVE_H_