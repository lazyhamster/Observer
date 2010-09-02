// pst.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ModuleDef.h"

using namespace pstsdk;
using namespace std;

static bool optExpandEmlFile = true;
static bool optHideEmptyFolders = false;

enum EntryType
{
	ETYPE_UNKNOWN,
	ETYPE_FOLDER,
	ETYPE_EML,
	ETYPE_MESSAGE_BODY,
	ETYPE_MESSAGE_HTML,
	ETYPE_ATTACHMENT
};

struct PstFileEntry 
{
	EntryType Type;
	wstring Name;
	wstring FullPath;
	__int64 Size;
	
	message *msgRef;
	attachment *attachRef;

	PstFileEntry() : Type(ETYPE_UNKNOWN), Size(0), msgRef(NULL), attachRef(NULL) {}
	PstFileEntry(const PstFileEntry &other)
	{
		Type = other.Type;
		Name = other.Name;
		FullPath = other.FullPath;
		Size = other.Size;
		msgRef = other.msgRef ? new message(*other.msgRef) : NULL;
		attachRef = other.attachRef ? new attachment(*other.attachRef) : NULL;
	}
	~PstFileEntry()
	{
		if (msgRef) delete msgRef;
		if (attachRef) delete attachRef;
	}
};

struct PstFileInfo
{
	pst* PstObject;
	vector<PstFileEntry> Entries;

	PstFileInfo(pst *obj) : PstObject(obj) {}
	~PstFileInfo() { Entries.clear(); delete PstObject; }
};

bool process_message(const message& m, PstFileInfo *fileInfoObj, const wstring parentPath)
{
	try {
		wstring strSubPath(parentPath);
		if (strSubPath.length() > 0)
			strSubPath.append(L"\\");

		// Create entry for message itself (file or folder depending on options)
		{
			wstring strFileName;
			if (m.has_subject())
				strFileName = m.get_subject() + L".eml";
			else
				strFileName = L"Message.eml";

			PstFileEntry fentry;
			fentry.Type = optExpandEmlFile ? ETYPE_FOLDER : ETYPE_EML;
			fentry.Name = strFileName;
			fentry.FullPath = strSubPath + strFileName;
			fentry.msgRef = new message(m);

			fileInfoObj->Entries.push_back(fentry);

			strSubPath.append(strFileName);
			strSubPath.append(L"\\");
		}

		// Create fake files for each part of the message
		if (optExpandEmlFile)
		{
			if (m.has_body())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_MESSAGE_BODY;
				fentry.Name = L"Message.txt";
				fentry.FullPath = strSubPath + L"Message.txt";
				fentry.Size = m.body_size();
				fentry.msgRef = new message(m);

				fileInfoObj->Entries.push_back(fentry);
			}

			if (m.has_html_body())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_MESSAGE_HTML;
				fentry.Name = L"Message.html";
				fentry.FullPath = strSubPath + L"Message.html";
				fentry.Size = m.html_body_size();
				fentry.msgRef = new message(m);

				fileInfoObj->Entries.push_back(fentry);
			}

			if (m.get_attachment_count() > 0)
				for (message::attachment_iterator att_iter = m.attachment_begin(); att_iter != m.attachment_end(); att_iter++)
				{
					const attachment &attach = *att_iter;

					PstFileEntry fentry;
					fentry.Type = ETYPE_ATTACHMENT;
					fentry.Name = attach.get_filename();
					fentry.FullPath = strSubPath + fentry.Name;
					fentry.Size = attach.content_size();
					fentry.msgRef = new message(m);
					fentry.attachRef = new attachment(attach);

					fileInfoObj->Entries.push_back(fentry);
				} // for
		}

		return true;
	}
	catch(key_not_found<prop_id>&)
	{
		//
	}

	return false;
}

bool process_folder(const folder& f, PstFileInfo *fileInfoObj, const wstring parentPath)
{
	size_t nMCount = f.get_message_count();
	size_t nFCount = f.get_subfolder_count();

	if (optHideEmptyFolders && nMCount == 0 && (f.sub_folder_begin() == f.sub_folder_end()))
		return true;

	wstring strSubPath(parentPath);
	wstring strFolderName = f.get_name();

	if (strFolderName.length() > 0)
	{
		if (strSubPath.length() > 0)
			strSubPath.append(L"\\");
		strSubPath.append(strFolderName);
		
		PstFileEntry entry;
		entry.Type = ETYPE_FOLDER;
		entry.Name = strFolderName;
		entry.FullPath = strSubPath;

		fileInfoObj->Entries.push_back(entry);
	}

	for(folder::message_iterator iter = f.message_begin(); iter != f.message_end(); ++iter)
	{
		if (!process_message(*iter, fileInfoObj, strSubPath)) // *iter is a message in this folder
			return false;
	}
	
	for(folder::folder_iterator iter = f.sub_folder_begin(); iter != f.sub_folder_end(); ++iter)
	{
		if (!process_folder(*iter, fileInfoObj, strSubPath))
			return false;
	}

	return true;
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

		folder pRoot = storeObj->open_root_folder();
		if (process_folder(pRoot, objInfo, L""))
		{
			*storage = (INT_PTR*) objInfo;

			memset(info, 0, sizeof(StorageGeneralInfo));
			wcscpy_s(info->Format, STORAGE_FORMAT_NAME_MAX_LEN, L"Outlook DB");
			wcscpy_s(info->Comment, STORAGE_PARAM_MAX_LEN, strDbName.c_str());
			wcscpy_s(info->Compression, STORAGE_PARAM_MAX_LEN, L"-");

			return TRUE;
		}

		delete storeObj;
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
	//item_data->ftLastWriteTime;

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
	DWORD nNumWritten;
	int ErrCode = SER_SUCCESS;

	switch (fentry.Type)
	{
		case ETYPE_MESSAGE_BODY:
			strBodyText = fentry.msgRef->get_body();
			WriteFile(hOutFile, strBodyText.c_str(), strBodyText.size() * sizeof(wchar_t), &nNumWritten, NULL);
			break;
		case ETYPE_MESSAGE_HTML:
			strBodyText = fentry.msgRef->get_html_body();
			WriteFile(hOutFile, strBodyText.c_str(), strBodyText.size() * sizeof(wchar_t), &nNumWritten, NULL);
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
					
					if (!WriteFile(hOutFile, inbuf, (DWORD) rsize, &nNumWritten, NULL))
						ErrCode = SER_ERROR_WRITE;
				}

				nstream->close();
			}
			break;
		default:
			ErrCode = SER_ERROR_READ;
			break;
	}
	
	CloseHandle(hOutFile);
	if (ErrCode != SER_SUCCESS)
		DeleteFile(params.destFilePath);

	return ErrCode;
}
