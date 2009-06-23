#include "StdAfx.h"
#include "NsisArchive.h"

CNsisArchive::CNsisArchive()
{
	m_stream = NULL;
	m_inArc = NULL;

	m_numFiles = 0;
	m_numDirectories = 0;
	m_totalSize = 0;
}

CNsisArchive::~CNsisArchive()
{
	Close();
}

int CNsisArchive::Open(const wchar_t* path)
{
	m_archiveName = path;
	
	CInFileStream* fileSpec = new CInFileStream;
    CMyComPtr<IInStream> file = fileSpec;
    
    if (!fileSpec->Open(m_archiveName))
		return FALSE;
	
	UInt64 nMaxCheckSize = 128 * 1024;
	CInArchive* inArc = new CInArchive();
	if (inArc->Open(file, &nMaxCheckSize) != S_OK)
	{
		delete inArc;
		return FALSE;
	}

	m_inArc = inArc;
	m_stream = fileSpec;

	m_numFiles = m_inArc->Items.Size();

	return TRUE;
}

void CNsisArchive::Close()
{
	if (m_inArc)
	{
		m_inArc->Clear();
		delete m_inArc;

		m_inArc = NULL;
		m_stream = NULL;
		m_archiveName.Empty();
	}

	m_numFiles = 0;
	m_numDirectories = 0;
	m_totalSize = 0;
}

UString CNsisArchive::getItemPath( CItem &item )
{
	UString name;
	if (item.IsUnicode)
	{
		if (item.PrefixU.IsEmpty() || (item.NameU.Find(L'\\') > 0))
			name = item.NameU;
		else
			name = item.PrefixU + L"\\" + item.NameU;
	}
	else
	{
		if (item.PrefixA.IsEmpty() || (item.NameA.Find('\\') > 0))
			name = MultiByteToUnicodeString(item.NameA);
		else
			name = MultiByteToUnicodeString(item.PrefixA + "\\" + item.NameA);
	}

	name.Replace(L"\\\\", L"\\");
	return name;
}

int CNsisArchive::GetItem(int itemIndex, WIN32_FIND_DATAW *itemData, wchar_t* itemPath, size_t itemPathSize)
{
	if (!m_inArc) return FALSE;
	
	CItem item = m_inArc->Items[itemIndex];
	
	UString name = getItemPath(item);
	wcscpy_s(itemPath, itemPathSize, name);

	memset(itemData, 0, sizeof(*itemData));
	
	const wchar_t* wszLastSlash = wcsrchr(name, '\\');
	if (wszLastSlash)
		wcscpy_s(itemData->cFileName, MAX_PATH, wszLastSlash + 1);
	else
		wcscpy_s(itemData->cFileName, MAX_PATH, name);
	if (item.MTime.dwHighDateTime > 0x01000000 && item.MTime.dwHighDateTime < 0xFF000000)
		itemData->ftLastWriteTime = item.MTime;

	// Get file size
	if (!GetUncompressedSize(item, itemData->nFileSizeLow) && !m_inArc->IsSolid && item.IsCompressed)
	{
		UInt32 fileSize = 0;
		RINOK(m_stream->Seek(m_inArc->GetPosOfNonSolidItem(itemIndex) + 4, STREAM_SEEK_SET, NULL));

		CByteBuffer byteBuf;
		const UInt32 kBufferLength = 1 << 16;
		byteBuf.SetCapacity(kBufferLength);
		Byte *buffer = byteBuf;

		bool useFilter;
		RINOK(m_inArc->Decoder.Init(
			EXTERNAL_CODECS_VARS
            m_stream, m_inArc->Method, m_inArc->FilterFlag, useFilter));

		UInt32 curSize = kBufferLength;
        size_t processedSize = curSize;
		while (processedSize > 0)
		{
			HRESULT res = m_inArc->Decoder.Read(buffer, &processedSize);
			RINOK(res);
			fileSize += processedSize;
		}

		if (fileSize > 0)
		{
			item.Size = fileSize;
			item.SizeIsDefined = true;

			itemData->nFileSizeLow = fileSize;
		}
	}

	return TRUE;
}

bool CNsisArchive::GetUncompressedSize(const CItem &item, DWORD &size)
{
  size = 0;
  if (item.SizeIsDefined)
     size = item.Size;
  else if (m_inArc->IsSolid && item.EstimatedSizeIsDefined)
     size  = item.EstimatedSize;
  else
    return false;
  return true;
}

int CNsisArchive::ExtractItemByName( const wchar_t* itemName, const wchar_t* destDir, const ExtractProcessCallbacks* epc )
{	
	int itemIndex = findItemIndex(itemName);
	if (itemIndex < 0) return SER_ERROR_READ;

	CItem &item = m_inArc->Items[itemIndex];
	
	//TODO: implement
	return 0;
}

int CNsisArchive::findItemIndex( const wchar_t* path )
{
	for (int i = 0; i < m_inArc->Items.Size(); i++)
	{
		CItem item = m_inArc->Items[i];
		UString itemPath = getItemPath(item);

		if (itemPath.Compare(path) == 0)
			return i;
	}
	
	return -1;
}
