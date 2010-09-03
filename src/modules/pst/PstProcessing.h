#ifndef PstProcessing_h__
#define PstProcessing_h__

using namespace pstsdk;

enum EntryType
{
	ETYPE_UNKNOWN,
	ETYPE_FOLDER,
	ETYPE_EML,
	ETYPE_MESSAGE_BODY,
	ETYPE_MESSAGE_HTML,
	ETYPE_ATTACHMENT,
	ETYPE_HEADER,
	ETYPE_PROPERTIES
};

#define PR_TRANSPORT_MESSAGE_HEADERS 0x007D
#define PR_CLIENT_SUBMIT_TIME 0x0039
#define PR_MESSAGE_DELIVERY_TIME 0x0E06

#define PR_CREATION_TIME 0x3007
#define PR_LAST_MODIFICATION_TIME 0x3008

struct PstFileEntry 
{
	EntryType Type;
	std::wstring Name;
	std::wstring FullPath;
	__int64 Size;
	FILETIME CreationTime;
	FILETIME LastModificationTime;

	message *msgRef;
	attachment *attachRef;

	PstFileEntry()
	{
		Type = ETYPE_UNKNOWN;
		Size = 0;
		msgRef = NULL;
		attachRef = NULL;
		memset(&CreationTime, 0, sizeof(CreationTime));
		memset(&LastModificationTime, 0, sizeof(LastModificationTime));
	}

	PstFileEntry(const PstFileEntry &other)
	{
		Type = other.Type;
		Name = other.Name;
		FullPath = other.FullPath;
		Size = other.Size;
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

#endif // PstProcessing_h__