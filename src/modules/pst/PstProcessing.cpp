#include "stdafx.h"
#include "PstProcessing.h"

using namespace std;

bool process_message(const message& m, PstFileInfo *fileInfoObj, const wstring &parentPath, int &NoNameCounter)
{
	try {
		wstring strSubPath(parentPath);
		if (strSubPath.length() > 0)
			strSubPath.append(L"\\");

		// Set same creation/modification time for all message files.
		FILETIME ftCreated = {0};
		FILETIME ftModified = {0};

		const property_bag &msgPropBag = m.get_property_bag();
		if (msgPropBag.prop_exists(PR_CLIENT_SUBMIT_TIME))
			ftCreated = msgPropBag.read_prop<FILETIME>(PR_CLIENT_SUBMIT_TIME);
		if (msgPropBag.prop_exists(PR_MESSAGE_DELIVERY_TIME))
			ftModified = msgPropBag.read_prop<FILETIME>(PR_MESSAGE_DELIVERY_TIME);

		// Create entry for message itself (file or folder depending on options)
		{
			wstring strFileName;
			if (m.has_subject())
			{
				strFileName = m.get_subject() + L".eml";
			}
			else
			{
				wchar_t buf[50] = {0};
				swprintf_s(buf, sizeof(buf) / sizeof(buf[0]), L"Message%d.eml", ++NoNameCounter);
				strFileName = buf;
			}

			PstFileEntry fentry;
			fentry.Type = fileInfoObj->ExpandEmlFile ? ETYPE_FOLDER : ETYPE_EML;
			fentry.Name = strFileName;
			fentry.FullPath = strSubPath + strFileName;
			fentry.msgRef = new message(m);
			fentry.CreationTime = ftCreated;
			fentry.LastModificationTime = ftModified;

			fileInfoObj->Entries.push_back(fentry);

			strSubPath.append(strFileName);
			strSubPath.append(L"\\");
		}

		// Create fake files for each part of the message
		if (fileInfoObj->ExpandEmlFile)
		{
			if (m.has_body())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_MESSAGE_BODY;
				fentry.Name = L"{Message}.txt";
				fentry.FullPath = strSubPath + fentry.Name;
				fentry.Size = m.body_size();
				fentry.msgRef = new message(m);
				fentry.CreationTime = ftCreated;
				fentry.LastModificationTime = ftModified;

				fileInfoObj->Entries.push_back(fentry);
			}

			if (m.has_html_body())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_MESSAGE_HTML;
				fentry.Name = L"{Message}.html";
				fentry.FullPath = strSubPath + fentry.Name;
				fentry.Size = m.html_body_size();
				fentry.msgRef = new message(m);
				fentry.CreationTime = ftCreated;
				fentry.LastModificationTime = ftModified;

				fileInfoObj->Entries.push_back(fentry);
			}

			// Fake file with mail headers
			if (msgPropBag.prop_exists(PR_TRANSPORT_MESSAGE_HEADERS))
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_HEADER;
				fentry.Name = L"{Msg-Header}.txt";
				fentry.FullPath = strSubPath + fentry.Name;
				fentry.Size = msgPropBag.size(PR_TRANSPORT_MESSAGE_HEADERS);
				fentry.msgRef = new message(m);
				fentry.CreationTime = ftCreated;
				fentry.LastModificationTime = ftModified;

				fileInfoObj->Entries.push_back(fentry);
			}

			// Fake file with list of message properties
			/*
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_PROPERTIES;
				fentry.Name = L"{Msg-Properties}.txt";
				fentry.FullPath = strSubPath + fentry.Name;
				fentry.Size = 0;
				fentry.msgRef = new message(m);

				fileInfoObj->Entries.push_back(fentry);
			}
			*/

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
					fentry.CreationTime = ftCreated;
					fentry.LastModificationTime = ftModified;

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

bool process_folder(const folder& f, PstFileInfo *fileInfoObj, const wstring &parentPath)
{
	size_t nMCount = f.get_message_count();

	if (fileInfoObj->HideEmptyFolders && (nMCount == 0) && (f.sub_folder_begin() == f.sub_folder_end()))
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

	int nNoNameCnt = 0;
	for(folder::message_iterator iter = f.message_begin(); iter != f.message_end(); ++iter)
	{
		if (!process_message(*iter, fileInfoObj, strSubPath, nNoNameCnt)) // *iter is a message in this folder
			return false;
	}

	for(folder::folder_iterator iter = f.sub_folder_begin(); iter != f.sub_folder_end(); ++iter)
	{
		if (!process_folder(*iter, fileInfoObj, strSubPath))
			return false;
	}

	return true;
}
