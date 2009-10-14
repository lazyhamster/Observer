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
	void Init(IInArchive *archiveHandler, const UString &directoryPath);

	UInt64 NumErrors;

	CArchiveExtractCallback() {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &directoryPath)
{
	NumErrors = 0;
	_archiveHandler = archiveHandler;
	_directoryPath = directoryPath;
	NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 /* size */)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */)
{
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
		NFile::NFind::CFileInfoW fi;
		if (NFile::NFind::FindFile(fullProcessedPath, fi))
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
		delete m_handler;
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

		if ( (m_handler->GetProperty(0, kpidSolid, &prop) == S_OK) && (prop.vt == VT_BOOL) )
			if (prop.boolVal) wcscat_s(m_archSubtype, STORAGE_SUBTYPE_NAME_MAX_LEN, L"Solid ");
		
		if ( (m_handler->GetProperty(0, kpidMethod, &prop) == S_OK) && (prop.vt != VT_EMPTY) )
			wcscat_s(m_archSubtype, STORAGE_SUBTYPE_NAME_MAX_LEN, prop.bstrVal);
	}

	return TRUE;
}

void CNsisArchive::Close()
{
	if (m_handler)
	{
		m_handler->Close();
		delete m_handler;

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

	// Some files have crap prefix (usually from $PLUGINDIR), it should be removed
	int nSignPos = name.Find(L"\\$");
	if (nSignPos >=0)
		name.Delete(0, nSignPos + 1);

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

int CNsisArchive::ExtractArcItem( const int itemIndex, const wchar_t* destDir, const ExtractProcessCallbacks* epc )
{	
	if (itemIndex < 0) return SER_ERROR_READ;

	CArchiveExtractCallback* callback = new CArchiveExtractCallback();
	callback->Init(m_handler, destDir);

	ProgressContext* pctx = (ProgressContext*) epc->signalContext;
	pctx->nCurrentFileProgress = 0;
	pctx->nCurrentFileIndex = itemIndex;

	epc->FileStart(pctx);
	
	UInt32 nIndex = itemIndex;
	HRESULT extResult = m_handler->Extract(&nIndex, 1, 0, callback);

	pctx->nProcessedBytes += GetItemSize(itemIndex);
	epc->FileEnd(pctx);

	if (extResult == S_OK)
		return SER_SUCCESS;

	return SER_ERROR_SYSTEM;
}

DWORD CNsisArchive::GetItemSize( int itemIndex )
{
	NWindows::NCOM::CPropVariant prop;
	if ( (m_handler->GetProperty(itemIndex, kpidSize, &prop) == S_OK) && (prop.vt != VT_EMPTY) )
		return prop.ulVal;

	return 0;
}