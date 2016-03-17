#include "stdafx.h"
#include "PstProcessing.h"
#include "rtfcomp.h"

using namespace std;

static bool IsValidPathChar(wchar_t ch)
{
	return (wcschr(L":*?\"<>|\\/", ch) == NULL);
}

static void RenameInvalidPathChars(wstring& input)
{
	for (size_t i = 0; i < input.length(); i++)
	{
		if (!IsValidPathChar(input[i]))
			input[i] = '_';
	}
}

static wstring GetDupTestFilename(wstring &baseName, int dupCounter)
{
	wchar_t testName[MAX_PATH] = {0};
	if (dupCounter > 0)
		swprintf_s(testName, MAX_PATH, L"%s[%d].eml", baseName.c_str(), dupCounter);
	else
		swprintf_s(testName, MAX_PATH, L"%s.eml", baseName.c_str());
	return testName;
}

bool process_message(const message& m, PstFileInfo *fileInfoObj, const wstring &parentPath, int &NoNameCounter)
{
	try {
		// Set same creation/modification time for all message files.
		FILETIME ftCreated = {0};
		FILETIME ftModified = {0};

		const property_bag &msgPropBag = m.get_property_bag();
		if (msgPropBag.prop_exists(PR_CLIENT_SUBMIT_TIME))
			ftCreated = msgPropBag.read_prop<FILETIME>(PR_CLIENT_SUBMIT_TIME);
		if (msgPropBag.prop_exists(PR_MESSAGE_DELIVERY_TIME))
			ftModified = msgPropBag.read_prop<FILETIME>(PR_MESSAGE_DELIVERY_TIME);

		wstring emlDirPath;

		// Create entry for message itself (file or folder depending on options)
		{
			wstring strBaseFileName;
			if (m.has_subject())
			{
				wstring subj = m.get_subject();
				if (subj.length() > 200) subj.erase(200);
				strBaseFileName = subj;
			}
			else
			{
				wchar_t buf[50] = {0};
				swprintf_s(buf, sizeof(buf) / sizeof(buf[0]), L"Message%05d", ++NoNameCounter);
				strBaseFileName = buf;
			}
			RenameInvalidPathChars(strBaseFileName);

			int dupCounter = 0;
			while (true)
			{
				bool dupFound = false;
				wstring testName = GetDupTestFilename(strBaseFileName, dupCounter);
				for (auto it = fileInfoObj->Entries.cbegin(); it != fileInfoObj->Entries.cend(); it++)
				{
					const PstFileEntry& prevEntry = *it;
					if ((prevEntry.Type == ETYPE_FOLDER || prevEntry.Type == ETYPE_EML) && (prevEntry.Name == testName) && (prevEntry.Folder == parentPath))
					{
						dupFound = true;
						break;
					}
				}
				if (!dupFound) break;
				
				dupCounter++;
			}

			wstring strFileName = GetDupTestFilename(strBaseFileName, dupCounter);
			
			PstFileEntry fentry;
			fentry.Type = fileInfoObj->ExpandEmlFile ? ETYPE_FOLDER : ETYPE_EML;
			fentry.Name = strFileName;
			fentry.Folder = parentPath;
			fentry.msgRef = new message(m);
			fentry.CreationTime = ftCreated;
			fentry.LastModificationTime = ftModified;

			fileInfoObj->Entries.push_back(fentry);

			emlDirPath = parentPath;
			if (emlDirPath.length() > 0)
				emlDirPath.append(L"\\");
			emlDirPath.append(strFileName);
		}

		// Create fake files for each part of the message
		if (fileInfoObj->ExpandEmlFile)
		{
			if (m.has_body())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_MESSAGE_BODY;
				fentry.Name = L"{Message}.txt";
				fentry.Folder = emlDirPath;
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
				fentry.Folder = emlDirPath;
				fentry.msgRef = new message(m);
				fentry.CreationTime = ftCreated;
				fentry.LastModificationTime = ftModified;

				fileInfoObj->Entries.push_back(fentry);
			}

			if (m.has_compressed_rtf_body())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_MESSAGE_RTF;
				fentry.Name = L"{Message}.rtf";
				fentry.Folder = emlDirPath;
				fentry.msgRef = new message(m);
				fentry.CreationTime = ftCreated;
				fentry.LastModificationTime = ftModified;

				fileInfoObj->Entries.push_back(fentry);
			}

			// Fake file with mail headers
			if (m.has_transport_headers())
			{
				PstFileEntry fentry;
				fentry.Type = ETYPE_HEADER;
				fentry.Name = L"{Msg-Header}.txt";
				fentry.Folder = emlDirPath;
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
				for (auto att_iter = m.attachment_begin(); att_iter != m.attachment_end(); att_iter++)
				{
					const attachment &attach = *att_iter;

					PstFileEntry fentry;
					fentry.Type = ETYPE_ATTACHMENT;
					try
					{
						fentry.Name = attach.get_filename();
						if (fentry.Name.length() >= MAX_PATH)
							fentry.Name.erase(MAX_PATH);
					}
					catch(key_not_found<prop_id>&)
					{
						fentry.Name = L"noname.dat";
					}
					fentry.Folder = emlDirPath;
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

	RenameInvalidPathChars(strFolderName);

	if (strFolderName.length() > 0)
	{
		if (strSubPath.length() > 0)
			strSubPath.append(L"\\");
		strSubPath.append(strFolderName);
		
		PstFileEntry entry;
		entry.Type = ETYPE_FOLDER;
		entry.Name = strFolderName;
		entry.Folder = parentPath;

		fileInfoObj->Entries.push_back(entry);
	}

	int nNoNameCnt = 0;
	for(auto iter = f.message_begin(); iter != f.message_end(); iter++)
	{
		const message& m = *iter;
		if (!process_message(m, fileInfoObj, strSubPath, nNoNameCnt)) // *iter is a message in this folder
			return false;
	}

	for(auto iter = f.sub_folder_begin(); iter != f.sub_folder_end(); iter++)
	{
		const folder& f = *iter;
		if (!process_folder(f, fileInfoObj, strSubPath))
			return false;
	}

	return true;
}

static ExtractResult dump_stream(hnid_stream_device &input, HANDLE output)
{
	prop_stream nstream(input);
	nstream->seek(0, ios_base::beg);

	ExtractResult ErrCode = ER_SUCCESS;
	streamsize rsize;
	DWORD nNumWritten;

	const size_t inbuf_size = 16 * 1024;
	char inbuf[inbuf_size];
	while (ErrCode == ER_SUCCESS)
	{
		rsize = nstream->read(inbuf, inbuf_size);
		if (rsize == -1) break; //EOF

		BOOL fWriteResult = WriteFile(output, inbuf, (DWORD) rsize, &nNumWritten, NULL);
		if (!fWriteResult)
		{
			ErrCode = ER_ERROR_WRITE;
			break;
		}
	}

	nstream->close();
	return ErrCode;
}

static BOOL DumpWString(std::wstring &str, HANDLE hOut, bool includeBOM = true)
{
	DWORD nNumWritten;
	BOOL fWriteResult;

	if (includeBOM)
	{
		fWriteResult = WriteFile(hOut, &UTF16_BOM, sizeof(UTF16_BOM), &nNumWritten, NULL);
		if (!fWriteResult) return fWriteResult;
	}
	DWORD nWriteSize = (DWORD) (str.size() * sizeof(wchar_t));
	fWriteResult = WriteFile(hOut, str.c_str(), nWriteSize, &nNumWritten, NULL);
	
	return fWriteResult;
}

//////////////////////////////////////////////////////////////////////////

__int64 PstFileEntry::GetSize() const
{
	switch (Type)
	{
	case ETYPE_MESSAGE_BODY:
		return msgRef->body_size();
	case ETYPE_MESSAGE_HTML:
		return msgRef->html_body_size();
	case ETYPE_MESSAGE_RTF:
		return GetRTFBodySize();
	case ETYPE_ATTACHMENT:
		return attachRef->content_size();
	case ETYPE_HEADER:
		return msgRef->transport_headers_size();
	default:
		return 0;
	}
}

ExtractResult PstFileEntry::ExtractContent(HANDLE hOut) const
{
	DWORD nNumWritten;
	BOOL fWriteResult = TRUE;
	const property_bag &propBag = msgRef->get_property_bag();
	
	switch (Type)
	{
	case ETYPE_MESSAGE_BODY:
		return DumpProp(PropMessageBodyPlainText, hOut);
	case ETYPE_MESSAGE_HTML:
		return DumpProp(PropMessageBodyHTML, hOut);
	case ETYPE_MESSAGE_RTF:
		{
			std::string strData;
			if (GetRTFBody(strData))
				fWriteResult = WriteFile(hOut, strData.c_str(), (DWORD) strData.size(), &nNumWritten, NULL);
			else
				return ER_ERROR_READ;
		}
		break;
	case ETYPE_HEADER:
		return DumpProp(PropMessageTransportHeaders, hOut);
	case ETYPE_ATTACHMENT:
		if (attachRef)
			return dump_stream(attachRef->open_byte_stream(), hOut);
		else
			return ER_ERROR_READ;
	case ETYPE_PROPERTIES:
		{
			std::wstring strBodyText = DumpMessageProperties();
			fWriteResult = DumpWString(strBodyText, hOut);
		}
		break;
	default:
		return ER_ERROR_READ;
	}

	return fWriteResult ? ER_SUCCESS : ER_ERROR_WRITE;
}

bool PstFileEntry::GetRTFBody(std::string &dest) const
{
	prop_stream nstream(msgRef->open_compressed_rtf_body_stream());
	nstream->seek(0, ios_base::beg);

	LZFUHDR hdr = {0};
	nstream->read((char*) &hdr, sizeof(hdr));

	DWORD dataSize = hdr.cbSize - 12;
	char* compData = (char*) malloc(dataSize);

	nstream->read(compData, dataSize);
	
	if (hdr.dwMagic == dwMagicUncompressedRTF)
	{
		dest.assign(compData);
	}
	else if (hdr.dwMagic == dwMagicCompressedRTF)
	{
		DWORD bufCrc = calculateWeakCrc32((uint8_t*)compData, dataSize);
		if (bufCrc == hdr.dwCRC)
		{
			char* decompData = (char*) malloc(hdr.cbRawSize);
			if (lzfu_decompress((uint8_t*)compData, dataSize, (uint8_t*)decompData, hdr.cbRawSize))
			{
				dest.assign(decompData, hdr.cbRawSize);
			}
			free(decompData);
		}
	}

	free(compData);
	
	return dest.size() > 0;
}

DWORD PstFileEntry::GetRTFBodySize() const
{
	prop_stream nstream(msgRef->open_compressed_rtf_body_stream());
	nstream->seek(0, ios_base::beg);

	LZFUHDR hdr = {0};
	nstream->read((char*) &hdr, sizeof(hdr));
	
	return hdr.cbRawSize;
}

std::wstring PstFileEntry::DumpMessageProperties() const
{
	std::wstringstream wss;

	const property_bag &propBag = msgRef->get_property_bag();
	std::vector<prop_id> propList = propBag.get_prop_list();
	for (auto citer = propList.cbegin(); citer != propList.cend(); citer++)
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

ExtractResult PstFileEntry::DumpProp(prop_id id, HANDLE hOut) const
{
	if (!msgRef) return ER_ERROR_READ;
	
	const property_bag &propBag = msgRef->get_property_bag();
	prop_type ptype = propBag.get_prop_type(id);

	ExtractResult rval = ER_SUCCESS;
	DWORD nNumWritten;

	switch(ptype)
	{
	case prop_type_string:
		{
			std::string strData = propBag.read_prop<std::string>(id);
			if (!WriteFile(hOut, strData.c_str(), (DWORD) strData.size(), &nNumWritten, NULL))
				rval = ER_ERROR_WRITE;
		}
		break;
	case prop_type_binary:
		{
			std::vector<byte> vData = propBag.read_prop<std::vector<byte>>(id);
			if (!WriteFile(hOut, vData.data(), (DWORD) vData.size(), &nNumWritten, NULL))
				return ER_ERROR_WRITE;
		}
		break;
	default:
		{
			std::wstring strData = propBag.read_prop<std::wstring>(id);
			rval = DumpWString(strData, hOut) ? ER_SUCCESS : ER_ERROR_WRITE;
		}
	}

	return rval;
}
