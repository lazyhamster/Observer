#include "StdAfx.h"
#include "NsisArchive.h"

#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"

#include "7zip/Common/FileStreams.h"

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

class COutStreamWithProgress : public ISequentialOutStream, public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ISequentialOutStream)

	// ISequentialOutStream
	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);

	~COutStreamWithProgress() { Close(); }

	Int64 GetProcessedSize() { return _processedBytes; }
	bool SetMTime(const FILETIME *mTime) { return IsRealFile() ? _file.SetMTime(mTime) : true; }

	bool Open(UString filePath, const ExtractProcessCallbacks *progressCallbacks)
	{
		_filePath = filePath;
		_progressCallbacks = progressCallbacks;
		_processedBytes = 0;
		return IsRealFile() ? _file.Open(filePath, CREATE_ALWAYS) : true;
	}
	void Close()
	{
		if (IsRealFile()) _file.Close();
		_filePath.Empty();
	}
private:
	UString _filePath;
	NWindows::NFile::NIO::COutFile _file;
	Int64 _processedBytes;
	const ExtractProcessCallbacks *_progressCallbacks;

	bool IsRealFile() { return !_filePath.IsEmpty(); }
};

HRESULT COutStreamWithProgress::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
	_processedBytes += size;
	if (!IsRealFile())
	{
		if (processedSize) *processedSize = size;
		return S_OK;
	}

	UInt32 bytesProcessed = 0;
	bool result = _file.Write(data, size, bytesProcessed);
	if (result)
	{
		if (processedSize) *processedSize = bytesProcessed;
		if (_progressCallbacks) _progressCallbacks->FileProgress(_progressCallbacks->signalContext, bytesProcessed);
		return S_OK;
	}
	return E_FAIL;
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
	UString _diskFilePath;   // full path to file on disk
	bool _extractMode;
	Int64 _completedSize;
	const ExtractProcessCallbacks *_progressCallbacks;

	struct CProcessedFileInfo
	{
		FILETIME MTime;
		UInt32 Attrib;
		bool isDir;
		bool AttribDefined;
		bool MTimeDefined;
	} _processedFileInfo;

	COutStreamWithProgress* _outFileStream;
	void CloseOutStream()
	{
		if (_outFileStream)
		{
			delete _outFileStream;
			_outFileStream = nullptr;
		}
	}

public:
	void Init(IInArchive *archiveHandler, const UString &destinationPath, const ExtractProcessCallbacks *progessCallbacks);
	Int64 GetCompleted() { return _completedSize; };

	CArchiveExtractCallback() {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &destinationPath, const ExtractProcessCallbacks *progessCallbacks)
{
	_archiveHandler = archiveHandler;
	_diskFilePath = destinationPath;
	_progressCallbacks = progessCallbacks;
	_outFileStream = nullptr;
	_completedSize = 0;
	memset(&_processedFileInfo, 0, sizeof(_processedFileInfo));
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * completeValue)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
	*outStream = 0;
	CloseOutStream();

	bool extractMode = askExtractMode != NArchive::NExtract::NAskMode::kExtract;

	if (extractMode)
	{
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
			switch (prop.vt)
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
	}

	if (_processedFileInfo.isDir)
	{
		NFile::NDir::CreateComplexDir(_diskFilePath);
	}
	else
	{
		if (extractMode && NFile::NFind::DoesFileExist(_diskFilePath))
		{
			if (!NFile::NDir::DeleteFileAlways(_diskFilePath))
			{
				return E_FAIL;
			}
		}

		COutStreamWithProgress* outStreamLoc = new COutStreamWithProgress();
		if (!outStreamLoc->Open(_diskFilePath, _progressCallbacks))
		{
			delete outStreamLoc;
			return E_FAIL;
		}
		_outFileStream = outStreamLoc;
		_completedSize = 0;
		*outStream = _outFileStream;
	}
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
	_extractMode = (askExtractMode == NArchive::NExtract::NAskMode::kExtract);
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	if (_outFileStream != nullptr)
	{
		_completedSize = _outFileStream->GetProcessedSize();
		if (_processedFileInfo.MTimeDefined)
			_outFileStream->SetMTime(&_processedFileInfo.MTime);
		CloseOutStream();
	}
	if (_extractMode && _processedFileInfo.AttribDefined && !_diskFilePath.IsEmpty())
		NFile::NDir::SetFileAttrib(_diskFilePath, _processedFileInfo.Attrib);

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
}

UString CNsisArchive::getItemPath( int itemIndex )
{
	NWindows::NCOM::CPropVariant prop;

	m_handler->GetProperty(itemIndex, kpidPath, &prop);
	
	UString name = prop.bstrVal;
	name.Replace(L"\\\\", L"\\");

	return name;
}

int CNsisArchive::GetItem(int itemIndex, StorageItemInfo* item_info)
{
	if (!m_handler) return FALSE;

	memset(item_info, 0, sizeof(*item_info));

	NWindows::NCOM::CPropVariant prop;

	UString name = getItemPath(itemIndex);
	wcscpy_s(item_info->Path, STRBUF_SIZE(item_info->Path), name);

	item_info->Size = GetItemSize(itemIndex);

	if ((m_handler->GetProperty(itemIndex, kpidMTime, &prop) == S_OK) && (prop.vt != VT_EMPTY))
		item_info->ModificationTime = prop.filetime;
	if ((m_handler->GetProperty(itemIndex, kpidPackSize, &prop) == S_OK) && (prop.vt != VT_EMPTY))
		item_info->PackedSize = prop.hVal.QuadPart;
	if ((m_handler->GetProperty(itemIndex, kpidAttrib, &prop) == S_OK) && (prop.vt != VT_EMPTY))
		item_info->Attributes = prop.uintVal;

	return TRUE;
}

int CNsisArchive::ExtractArcItem( const int itemIndex, const wchar_t* destFilePath, const ExtractProcessCallbacks* epc )
{	
	if (itemIndex < 0) return SER_ERROR_READ;

	CArchiveExtractCallback* callback = new CArchiveExtractCallback();
	CMyComPtr<IArchiveExtractCallback> extractCallback(callback);
	callback->Init(m_handler, destFilePath, epc);

	UInt32 nIndex = itemIndex;
	HRESULT extResult = m_handler->Extract(&nIndex, 1, 0, callback);

	if (extResult == S_OK)
		return SER_SUCCESS;
	else if (extResult == E_ABORT)
		return SER_USERABORT;

	return SER_ERROR_SYSTEM;
}

__int64 CNsisArchive::GetItemSize( int itemIndex )
{
	NWindows::NCOM::CPropVariant prop;
	if ( (m_handler->GetProperty(itemIndex, kpidSize, &prop) == S_OK) && (prop.vt != VT_EMPTY) )
		return prop.hVal.QuadPart;

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
	}

	return res;
}

void CNsisArchive::GetCompressionName(wchar_t* nameBuf, size_t nameBufSize)
{
	if (m_handler && (m_numFiles > 0))
	{
		NWindows::NCOM::CPropVariant prop;

		if ((m_handler->GetArchiveProperty(kpidSolid, &prop) == S_OK) && (prop.vt == VT_BOOL))
			if (prop.boolVal) wcscat_s(nameBuf, nameBufSize, L"Solid ");

		if ((m_handler->GetArchiveProperty(kpidMethod, &prop) == S_OK) && (prop.vt != VT_EMPTY))
			wcscat_s(nameBuf, nameBufSize, prop.bstrVal);
		else
			wcscat_s(nameBuf, nameBufSize, L"Unknown");
	}
	else
	{
		wcscpy_s(nameBuf, nameBufSize, L"None");
	}
}

void CNsisArchive::GetSubtypeName(wchar_t* nameBuf, size_t nameBufSize)
{
	NWindows::NCOM::CPropVariant prop;
	if ((m_handler->GetArchiveProperty(kpidSubType, &prop) == S_OK) && (prop.vt != VT_EMPTY))
		wcscpy_s(nameBuf, nameBufSize, prop.bstrVal);
	else
		wcscpy_s(nameBuf, nameBufSize, L"NSIS");
}

int CNsisArchive::GetItemsCount()
{
	UInt32 numItems = 0;
	m_handler->GetNumberOfItems(&numItems);
	return numItems;
}
