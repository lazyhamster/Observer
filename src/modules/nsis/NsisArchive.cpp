#include "StdAfx.h"
#include "NsisArchive.h"

#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"

using namespace NWindows;

//////////////////////////////////////////////////////////////
// Archive Extracting callback class

static const wchar_t *kEmptyFileAlias = L"[Content]";

static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
	NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, propID, &prop));
	if (prop.vt == VT_BOOL)
		result = VARIANT_BOOLToBool(prop.boolVal);
	else if (prop.vt == VT_EMPTY)
		result = false;
	else
		return E_FAIL;
	return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
	return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

static UInt64 ConvertPropVariantToUInt64(const PROPVARIANT &prop)
{
	switch (prop.vt)
	{
	case VT_UI1: return prop.bVal;
	case VT_UI2: return prop.uiVal;
	case VT_UI4: return prop.ulVal;
	case VT_UI8: return (UInt64)prop.uhVal.QuadPart;
	default:
		return 0;
	}
}

class CArchiveExtractCallback:
	public IArchiveExtractCallback,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(IArchiveExtractCallback)

	// IProgress
	STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode);
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

private:
	CMyComPtr<IInArchive> _archiveHandler;
	UString _directoryPath;  // Output directory
	UString _filePath;       // name inside archive
	UString _diskFilePath;   // full path to file on disk
	bool _extractMode;
	UInt64 _completed, _prevCompleted;
	UInt64 _totalSize;
	const ExtractProcessCallbacks *_progressCallbacks;

	struct CProcessedFileInfo
	{
		FILETIME MTime;
		UInt32 Attrib;
		bool isDir;
		bool AttribDefined;
		bool MTimeDefined;
	} _processedFileInfo;

	COutFileStream *_outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;

public:
	void Init(IInArchive *archiveHandler, const UString &directoryPath, const ExtractProcessCallbacks *progessCallbacks);
	UInt64 GetCompleted() { return _completed; };

	UInt64 NumErrors;
	
	CArchiveExtractCallback() {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &directoryPath, const ExtractProcessCallbacks *progessCallbacks)
{
	NumErrors = 0;
	_archiveHandler = archiveHandler;
	_directoryPath = directoryPath;
	NFile::NName::NormalizeDirPathPrefix(_directoryPath);

	_totalSize = 0;
	_completed = 0;
	_prevCompleted = 0;
	_progressCallbacks = progessCallbacks;
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
	_totalSize = size;
	_completed = 0;
	_prevCompleted = 0;
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * completeValue)
{
	_prevCompleted = _completed;
	_completed = *completeValue;

	if (_progressCallbacks)
	{
		_progressCallbacks->FileProgress(_progressCallbacks->signalContext, _completed - _prevCompleted);
	}
	
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
	*outStream = 0;
	_outFileStream.Release();

	{
		// Get Name
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidPath, &prop));

		UString fullPath;
		if (prop.vt == VT_EMPTY)
			fullPath = kEmptyFileAlias;
		else
		{
			if (prop.vt != VT_BSTR)
				return E_FAIL;
			fullPath = prop.bstrVal;
		}
		_filePath = fullPath;
	}

	if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
		return S_OK;

	{
		// Get Attrib
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop));
		if (prop.vt == VT_EMPTY)
		{
			_processedFileInfo.Attrib = 0;
			_processedFileInfo.AttribDefined = false;
		}
		else
		{
			if (prop.vt != VT_UI4)
				return E_FAIL;
			_processedFileInfo.Attrib = prop.ulVal;
			_processedFileInfo.AttribDefined = true;
		}
	}

	RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.isDir));

	{
		// Get Modified Time
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop));
		_processedFileInfo.MTimeDefined = false;
		switch(prop.vt)
		{
		case VT_EMPTY:
			// _processedFileInfo.MTime = _utcMTimeDefault;
			break;
		case VT_FILETIME:
			_processedFileInfo.MTime = prop.filetime;
			_processedFileInfo.MTimeDefined = true;
			break;
		default:
			return E_FAIL;
		}

	}
	{
		// Get Size
		NCOM::CPropVariant prop;
		RINOK(_archiveHandler->GetProperty(index, kpidSize, &prop));
		bool newFileSizeDefined = (prop.vt != VT_EMPTY);
		UInt64 newFileSize;
		if (newFileSizeDefined)
			newFileSize = ConvertPropVariantToUInt64(prop);
	}

	UString fullProcessedPath = _directoryPath;
	int slashPos = _filePath.ReverseFind(WCHAR_PATH_SEPARATOR);
	if (slashPos >= 0)
		fullProcessedPath += _filePath.Right(_filePath.Length() - slashPos - 1);
	else
		fullProcessedPath += _filePath;

	_diskFilePath = fullProcessedPath;

	if (_processedFileInfo.isDir)
	{
		NFile::NDirectory::CreateComplexDirectory(fullProcessedPath);
	}
	else
	{
		if (NFile::NFind::DoesFileExist(fullProcessedPath))
		{
			if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
			{
				return E_ABORT;
			}
		}

		_outFileStreamSpec = new COutFileStream;
		CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
		if (!_outFileStreamSpec->Open(fullProcessedPath, CREATE_ALWAYS))
		{
			return E_ABORT;
		}
		_outFileStream = outStreamLoc;
		*outStream = outStreamLoc.Detach();
	}
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
	_extractMode = true;
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	if (_outFileStream != NULL)
	{
		if (_processedFileInfo.MTimeDefined)
			_outFileStreamSpec->SetMTime(&_processedFileInfo.MTime);
		RINOK(_outFileStreamSpec->Close());
	}
	_outFileStream.Release();
	if (_extractMode && _processedFileInfo.AttribDefined)
		NFile::NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attrib);

	return S_OK;
}


//////////////////////////////////////////////////////////////
// Archive Operations class

CNsisArchive::CNsisArchive()
{
	m_stream = NULL;
	m_handler = NULL;

	m_numFiles = 0;
	m_numDirectories = 0;
	m_totalSize = 0;

	memset(m_archSubtype, 0, sizeof(m_archSubtype));
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
	
	UInt64 nMaxCheckSize = 1024 * 1024;
	m_handler = new CHandler();
	if (m_handler->Open(file, &nMaxCheckSize, NULL) != S_OK)
	{
		m_handler = NULL;
		return FALSE;
	}

	m_stream = fileSpec;

	UInt32 nNumFiles = 0;
	m_handler->GetNumberOfItems(&nNumFiles);
	m_numFiles = nNumFiles;

	if (nNumFiles > 0)
	{
		NWindows::NCOM::CPropVariant prop;

		if ( (m_handler->GetArchiveProperty(kpidSolid, &prop) == S_OK) && (prop.vt == VT_BOOL) )
			if (prop.boolVal) wcscat_s(m_archSubtype, STORAGE_PARAM_MAX_LEN, L"Solid ");
		
		if ( (m_handler->GetArchiveProperty(kpidMethod, &prop) == S_OK) && (prop.vt != VT_EMPTY) )
			wcscat_s(m_archSubtype, STORAGE_PARAM_MAX_LEN, prop.bstrVal);
	}

	return TRUE;
}

void CNsisArchive::Close()
{
	if (m_handler)
	{
		m_handler->Close();

		m_handler = NULL;
		m_stream = NULL;
		m_archiveName.Empty();
	}

	m_numFiles = 0;
	m_numDirectories = 0;
	m_totalSize = 0;
	memset(m_archSubtype, 0, sizeof(m_archSubtype));
}

UString CNsisArchive::getItemPath( int itemIndex )
{
	NWindows::NCOM::CPropVariant prop;

	m_handler->GetProperty(itemIndex, kpidPath, &prop);
	
	UString name = prop.bstrVal;
	name.Replace(L"\\\\", L"\\");

	return name;
}

int CNsisArchive::GetItem(int itemIndex, WIN32_FIND_DATAW *itemData, wchar_t* itemPath, size_t itemPathSize)
{
	if (!m_handler) return FALSE;

	memset(itemData, 0, sizeof(*itemData));

	NWindows::NCOM::CPropVariant prop;
	
	UString name = getItemPath(itemIndex);
	wcscpy_s(itemPath, itemPathSize, name);
	
	wchar_t* wszSlash = wcsrchr(itemPath, '\\');
	if (wszSlash)
		wcscpy_s(itemData->cFileName, MAX_PATH, wszSlash + 1);
	else
		wcscpy_s(itemData->cFileName, MAX_PATH, itemPath);
	
	itemData->nFileSizeLow = GetItemSize(itemIndex);
	
	if ( (m_handler->GetProperty(itemIndex, kpidMTime, &prop) == S_OK) && (prop.vt != VT_EMPTY) )
		itemData->ftLastWriteTime = prop.filetime;

	return TRUE;
}

int CNsisArchive::ExtractArcItem( const int itemIndex, const wchar_t* destFilePath, const ExtractProcessCallbacks* epc )
{	
	if (itemIndex < 0) return SER_ERROR_READ;

	UString destDir = destFilePath;
	int nLastSlashPos = destDir.ReverseFind('\\');
	if (nLastSlashPos >= 0)
		destDir = destDir.Left(nLastSlashPos + 1);

	CArchiveExtractCallback* callback = new CArchiveExtractCallback();
	CMyComPtr<IArchiveExtractCallback> extractCallback(callback);
	callback->Init(m_handler, destDir, epc);

	UInt32 nIndex = itemIndex;
	HRESULT extResult = m_handler->Extract(&nIndex, 1, 0, callback);

	if (extResult == S_OK)
		return SER_SUCCESS;

	return SER_ERROR_SYSTEM;
}

DWORD CNsisArchive::GetItemSize( int itemIndex )
{
	NWindows::NCOM::CPropVariant prop;
	if ( (m_handler->GetProperty(itemIndex, kpidSize, &prop) == S_OK) && (prop.vt != VT_EMPTY) )
		return prop.ulVal;

	DWORD res = 0;

	// For non-solid archives only
	if ( (m_handler->GetArchiveProperty(kpidSolid, &prop) == S_OK) && (prop.vt == VT_BOOL) && (!prop.boolVal) )
	{
		CArchiveExtractCallback* callback = new CArchiveExtractCallback();
		CMyComPtr<IArchiveExtractCallback> extractCallback(callback);
		callback->Init(m_handler, L"", NULL);

		UInt32 nIndex = itemIndex;
		HRESULT extResult = m_handler->Extract(&nIndex, 1, 1, callback);
		if (extResult == S_OK)
			res = (DWORD) callback->GetCompleted();

		// Cache value for future use
		NWindows::NCOM::CPropVariant propSet((UInt32) res);
		m_handler->SetProperty(itemIndex, kpidSize, &propSet);
	}

	return res;
}

int CNsisArchive::GetItemsCount()
{
	UInt32 numItems = 0;
	m_handler->GetNumberOfItems(&numItems);
	return numItems;
}
