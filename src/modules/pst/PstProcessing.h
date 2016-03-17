#ifndef PstProcessing_h__
#define PstProcessing_h__

using namespace pstsdk;

const uint16_t UTF16_BOM = 0xFEFF;

enum EntryType
{
	ETYPE_UNKNOWN,
	ETYPE_FOLDER,
	ETYPE_EML,
	ETYPE_MESSAGE_BODY,
	ETYPE_MESSAGE_HTML,
	ETYPE_MESSAGE_RTF,
	ETYPE_ATTACHMENT,
	ETYPE_HEADER,
	ETYPE_PROPERTIES
};

enum ExtractResult
{
	ER_SUCCESS,
	ER_ERROR_READ,
	ER_ERROR_WRITE
};

#define PR_CLIENT_SUBMIT_TIME 0x0039
#define PR_MESSAGE_DELIVERY_TIME 0x0E06

struct PstFileEntry 
{
	EntryType Type;
	std::wstring Name;
	std::wstring Folder;
	FILETIME CreationTime;
	FILETIME LastModificationTime;

	message *msgRef;
	attachment *attachRef;

	PstFileEntry()
	{
		Type = ETYPE_UNKNOWN;
		msgRef = NULL;
		attachRef = NULL;
		memset(&CreationTime, 0, sizeof(CreationTime));
		memset(&LastModificationTime, 0, sizeof(LastModificationTime));
	}

	PstFileEntry(const PstFileEntry &other)
	{
		Type = other.Type;
		Name = other.Name;
		Folder = other.Folder;
		msgRef = other.msgRef ? new message(*other.msgRef) : NULL;
		attachRef = other.attachRef ? new attachment(*other.attachRef) : NULL;
		CreationTime = other.CreationTime;
		LastModificationTime = other.LastModificationTime;
	}

	~PstFileEntry()
	{
		if (msgRef) delete msgRef;
		if (attachRef) delete attachRef;
	}

	std::wstring GetFullPath() const
	{
		if (Folder.size() > 0)
			return Folder + L"\\" + Name;
		else
			return Name;
	}

	__int64 GetSize() const;
	ExtractResult ExtractContent(HANDLE hOut) const;

	bool GetRTFBody(std::string &dest) const;

private:
	DWORD GetRTFBodySize() const;
	std::wstring DumpMessageProperties() const;
	ExtractResult DumpProp(prop_id id, HANDLE hOut) const;
};

struct PstFileInfo
{
	pst* PstObject;
	std::vector<PstFileEntry> Entries;

	bool HideEmptyFolders;
	bool ExpandEmlFile;

	PstFileInfo(pst *obj) : PstObject(obj), HideEmptyFolders(false), ExpandEmlFile(true) {}
	~PstFileInfo() { Entries.clear(); delete PstObject; }
};

bool process_message(const message& m, PstFileInfo *fileInfoObj, const std::wstring &parentPath);
bool process_folder(const folder& f, PstFileInfo *fileInfoObj, const std::wstring &parentPath);

std::wstring DumpMessageProperties(const PstFileEntry &entry);

#endif // PstProcessing_h__