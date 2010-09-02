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
	ETYPE_HEADER
};

struct PstFileEntry 
{
	EntryType Type;
	std::wstring Name;
	std::wstring FullPath;
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
	std::vector<PstFileEntry> Entries;

	bool HideEmptyFolders;
	bool ExpandEmlFile;

	PstFileInfo(pst *obj) : PstObject(obj), HideEmptyFolders(false), ExpandEmlFile(true) {}
	~PstFileInfo() { Entries.clear(); delete PstObject; }
};

bool process_message(const message& m, PstFileInfo *fileInfoObj, const std::wstring &parentPath);
bool process_folder(const folder& f, PstFileInfo *fileInfoObj, const std::wstring &parentPath);

#endif // PstProcessing_h__