// pst.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <sstream>
#include "ModuleDef.h"
#include "PstProcessing.h"

using namespace std;

static bool optExpandEmlFile = true;
static bool optHideEmptyFolders = false;

//////////////////////////////////////////////////////////////////////////

static wstring DumpProperties(const PstFileEntry &entry)
{
	wstringstream wss;
				
	const property_bag &propBag = entry.msgRef->get_property_bag();
	vector<prop_id> propList = propBag.get_prop_list();
	for (vector<prop_id>::const_iterator citer = propList.begin(); citer != propList.end(); citer++)
	{
		prop_id pid = *citer;
		prop_type ptype = propBag.get_prop_type(pid);
		
		wss << L"[" << pid << L"] -> ";
		switch (ptype)
		{
			case prop_type_string:
			case prop_type_wstring:
				wss << propBag.read_prop<wstring>(pid) << endl;
				break;
			case prop_type_longlong:
				wss << propBag.read_prop<long long>(pid) << endl;
				break;
			case prop_type_long:
				wss << propBag.read_prop<long>(pid) << endl;
				break;
			case prop_type_short:
				wss << propBag.read_prop<short>(pid) << endl;
				break;
			case prop_type_systime:
				{
					FILETIME timeVal = propBag.read_prop<FILETIME>(pid);
					wss << L"[Time Value]" << endl;
				}
				break;
			default:
				wss << L"Unknown Type (" << ptype << L")" << endl;
				break;
		}
	}

	wstring result = wss.str();
	return result;
}

//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(const wchar_t* settings)
{
	return TRUE;
}

int MODULE_EXPORT OpenStorage(const wchar_t *path, INT_PTR **storage, StorageGeneralInfo* info)
{
	wstring strPath(path);
	pst *storeObj = NULL;
	
	try
	{
		storeObj = new pst(strPath);
		wstring strDbName = storeObj->get_name();
		
		PstFileInfo *objInfo = new PstFileInfo(storeObj);
		objInfo->HideEmptyFolders = optHideEmptyFolders;
		objInfo->ExpandEmlFile = optExpandEmlFile;

		folder pRoot = storeObj->open_root_folder();
		if (process_folder(pRoot, objInfo, L""))
		{
			*storage = (INT_PTR*) objInfo;

			memset(info, 0, sizeof(StorageGeneralInfo));
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Outlook DB");
			wcsncpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, strDbName.c_str(), _TRUNCATE);
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

			return TRUE;
		}

		delete objInfo;
	}
	catch(...) {}
	
	return FALSE;
}

void MODULE_EXPORT CloseStorage(INT_PTR *storage)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (file)
	{
		delete file;
	}
}

int MODULE_EXPORT GetStorageItem(INT_PTR* storage, int item_index, LPWIN32_FIND_DATAW item_data, wchar_t* item_path, size_t path_size)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (!file) return GET_ITEM_ERROR;

	if (item_index < 0) return GET_ITEM_ERROR;
	if (item_index >= (int) file->Entries.size())
		return GET_ITEM_NOMOREITEMS;

	const PstFileEntry &fentry = file->Entries[item_index];
	wcscpy_s(item_path, path_size, fentry.FullPath.c_str());

	memset(item_data, 0, sizeof(WIN32_FIND_DATAW));
	wcscpy_s(item_data->cFileName, MAX_PATH, fentry.Name.c_str());
	wcscpy_s(item_data->cAlternateFileName, 14, L"");
	item_data->dwFileAttributes = (fentry.Type == ETYPE_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	item_data->nFileSizeLow = (DWORD) fentry.Size;
	item_data->nFileSizeHigh = (DWORD) (fentry.Size >> 32);
	item_data->ftLastWriteTime = fentry.LastModificationTime;
	item_data->ftCreationTime = fentry.CreationTime;

	return GET_ITEM_OK;
}

int MODULE_EXPORT ExtractItem(INT_PTR *storage, ExtractOperationParams params)
{
	PstFileInfo* file = (PstFileInfo*) storage;
	if (!file) return SER_ERROR_SYSTEM;

	if (params.item < 0 || params.item >= (int) file->Entries.size())
		return SER_ERROR_SYSTEM;

	HANDLE hOutFile = CreateFile(params.destFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hOutFile == INVALID_HANDLE_VALUE) return SER_ERROR_WRITE;

	const PstFileEntry &fentry = file->Entries[params.item];

	wstring strBodyText;
	DWORD nNumWritten, nWriteSize;
	int ErrCode = SER_SUCCESS;
	BOOL fWriteResult = TRUE;

	switch (fentry.Type)
	{
		case ETYPE_MESSAGE_BODY:
			strBodyText = fentry.msgRef->get_body();
			nWriteSize = (DWORD) (strBodyText.size() * sizeof(wchar_t));
			fWriteResult = WriteFile(hOutFile, strBodyText.c_str(), nWriteSize, &nNumWritten, NULL);
			break;
		case ETYPE_MESSAGE_HTML:
			strBodyText = fentry.msgRef->get_html_body();
			nWriteSize = (DWORD) (strBodyText.size() * sizeof(wchar_t));
			fWriteResult = WriteFile(hOutFile, strBodyText.c_str(), nWriteSize, &nNumWritten, NULL);
			break;
		case ETYPE_ATTACHMENT:
			if (fentry.attachRef)
			{
				prop_stream nstream(fentry.attachRef->open_byte_stream());
				nstream->seek(0, ios_base::beg);

				streamsize rsize;
				const size_t inbuf_size = 16 * 1024;
				char inbuf[inbuf_size];
				while (ErrCode == SER_SUCCESS)
				{
					rsize = nstream->read(inbuf, inbuf_size);
					if (rsize == -1) break; //EOF
					
					fWriteResult = WriteFile(hOutFile, inbuf, (DWORD) rsize, &nNumWritten, NULL);
					if (!fWriteResult) break;
				}

				nstream->close();
			}
			else
			{
				ErrCode = SER_ERROR_READ;
			}
			break;
		case ETYPE_PROPERTIES:
			strBodyText = DumpProperties(fentry);
			nWriteSize = (DWORD) (strBodyText.size() * sizeof(wchar_t));
			fWriteResult = WriteFile(hOutFile, strBodyText.c_str(), nWriteSize, &nNumWritten, NULL);
			break;
		case ETYPE_HEADER:
			{
				const property_bag &propBag = fentry.msgRef->get_property_bag();
				strBodyText = propBag.read_prop<wstring>(PR_TRANSPORT_MESSAGE_HEADERS);
				nWriteSize = (DWORD) (strBodyText.size() * sizeof(wchar_t));
				fWriteResult = WriteFile(hOutFile, strBodyText.c_str(), nWriteSize, &nNumWritten, NULL);
			}
			break;
		default:
			ErrCode = SER_ERROR_READ;
			break;
	}

	CloseHandle(hOutFile);
	
	if (!fWriteResult) ErrCode = SER_ERROR_WRITE;
	if (ErrCode != SER_SUCCESS)
		DeleteFile(params.destFilePath);

	return ErrCode;
}
