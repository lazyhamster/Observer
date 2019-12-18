#ifndef _NSIS_ARCHIVE_H_
#define _NSIS_ARCHIVE_H_

#include "ModuleDef.h"

#include "NsisIn.h"
#include "NsisHandler.h"

using namespace NArchive::NNsis;

class CNsisArchive
{
private:
	UString m_archiveName;
	CMyComPtr<IInStream> m_stream;
	CMyComPtr<CHandler> m_handler;

	int m_numFiles;
	int m_numDirectories;
	__int64 m_totalSize;

	UString getItemPath(int itemIndex);

public:
	CNsisArchive();
	~CNsisArchive();

	int Open(const wchar_t* path);
	void Close();

	int GetTotalFiles() { return m_numFiles; };
	int GetTotalDirectories() { return m_numDirectories; };
	__int64 GetTotalSize() { return m_totalSize; };
	void GetCompressionName(wchar_t* nameBuf, size_t nameBufSize);
	void GetSubtypeName(wchar_t* nameBuf, size_t nameBufSize);

	int GetItemsCount();
	int GetItem(int itemIndex, StorageItemInfo* item_info);
	__int64 GetItemSize(int itemIndex);

	int ExtractArcItem(const int itemIndex, const wchar_t* destFilePath, const ExtractProcessCallbacks* epc);
};

#endif //_NSIS_ARCHIVE_H_