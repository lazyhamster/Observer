#include "StdAfx.h"
#include "ContentStructs.h"

#define KILLSTR(wstr) if(wstr){free(wstr); wstr = NULL;}

void SplitEntryWithAlloc(wchar_t* source, wchar_t delim, wchar_t* &shortName, wchar_t* &longName)
{
	wchar_t *delimPos = wcschr(source, delim);
	
	if (delimPos)
	{
		*delimPos = 0;
		shortName = _wcsdup(source);
		longName = _wcsdup(delimPos + 1);
		*delimPos = delim;
	}
	else
	{
		// Short name can not be more then 14 symbols
		shortName = (wcslen(source) <= MAX_SHORT_NAME_LEN) ? _wcsdup(source) : _wcsdup(L"");
		longName = _wcsdup(source);
	}
}

//////////////////////////////////////////////////////////////////////////
//                           DirectoryNode
//////////////////////////////////////////////////////////////////////////

DirectoryNode::DirectoryNode()
{
	Key = NULL;
	ParentKey = NULL;
	Parent = NULL;

	SourceName = NULL;
	SourceShortName = NULL;
	TargetName = NULL;
	TargetShortName = NULL;
	IsSpecial = false;

	SequentialIndex = -1;
}

DirectoryNode::~DirectoryNode()
{
	KILLSTR(Key);
	KILLSTR(ParentKey);
	KILLSTR(SourceName);
	KILLSTR(SourceShortName);
	KILLSTR(TargetName);
	KILLSTR(TargetShortName);

	for (size_t i = 0; i < Files.size(); i++)
		delete Files[i];
	for (size_t i = 0; i < SubDirs.size(); i++)
		delete SubDirs[i];
}

bool DirectoryNode::Init(DirectoryEntry *entry, bool substDotTargetWithSource)
{
	if (!entry || !entry->Key || !*entry->Key)
		return false;

	Key = _wcsdup(entry->Key);
	if (entry->ParentKey[0])
		ParentKey = _wcsdup(entry->ParentKey);
	
	wchar_t *colonPos = wcschr(entry->DefaultName, ':');
	if (colonPos)
	{
		*colonPos = 0;
		if (substDotTargetWithSource && !wcscmp(entry->DefaultName, L".") && wcscmp(colonPos + 1, L"."))
		{
			// This is for special folders when using . as target path is undesirable
			// If target is . but source is not then use source in both name pairs
			SplitEntryWithAlloc(colonPos + 1, '|', TargetShortName, TargetName);
			SplitEntryWithAlloc(colonPos + 1, '|', SourceShortName, SourceName);
		}
		else
		{
			SplitEntryWithAlloc(entry->DefaultName, '|', TargetShortName, TargetName);
			SplitEntryWithAlloc(colonPos + 1, '|', SourceShortName, SourceName);
		}
		*colonPos = ':';
	}
	else
	{
		SplitEntryWithAlloc(entry->DefaultName, '|', TargetShortName, TargetName);
	}
	
	return true;
}

void DirectoryNode::AddSubdir(DirectoryNode *child)
{
	SubDirs.push_back(child);
	child->Parent = this;
}

int DirectoryNode::GetFilesCount()
{
	int nResult = Files.size();
	for (size_t i = 0; i < SubDirs.size(); i++)
	{
		DirectoryNode* subdir = SubDirs[i];
		nResult += subdir->GetFilesCount();
	}
	return nResult;
}

int DirectoryNode::GetSubDirsCount()
{
	int nResult = SubDirs.size();
	for (size_t i = 0; i < SubDirs.size(); i++)
	{
		DirectoryNode* subdir = SubDirs[i];
		nResult += subdir->GetSubDirsCount();
	}
	return nResult;
}

DWORD DirectoryNode::GetSytemAttributes()
{
	return FILE_ATTRIBUTE_DIRECTORY;
}

std::wstring DirectoryNode::GetSourcePath()
{
	wstring strResult;
	if (Parent)
	{
		strResult = Parent->GetSourcePath();
		if (strResult.size() > 0)
			strResult.append(L"\\");
	}
	if (SourceName)
		strResult.append(SourceName);
	else if (TargetName && wcscmp(Key, L"TARGETDIR"))
		strResult.append(TargetName);

	return strResult;
}

std::wstring DirectoryNode::GetTargetPath()
{
	wstring strResult;
	if (Parent)
	{
		strResult = Parent->GetTargetPath();
		if (strResult.size() > 0)
			strResult.append(L"\\");
	}
	if (TargetName /*&& wcscmp(Key, L"TARGETDIR")*/)
		strResult.append(TargetName);

	return strResult;
}

void DirectoryNode::AppendNumberToName(int val)
{
	size_t nNewLen = wcslen(TargetName) + 5;  // 1 for comma, 1 for 0-term, 3 for digits
	wchar_t *newName = (wchar_t *) malloc(nNewLen * sizeof(wchar_t));
	swprintf_s(newName, nNewLen, L"%s,%d", TargetName, val);

	KILLSTR(TargetName);
	TargetName = newName;
}

__int64 DirectoryNode::GetTotalSize()
{
	__int64 nResult = 0;

	for (size_t i = 0; i < SubDirs.size(); i++)
	{
		DirectoryNode* subdir = SubDirs[i];
		nResult += subdir->GetTotalSize();
	}

	for (size_t i = 0; i < Files.size(); i++)
	{
		FileNode* file = Files[i];
		nResult += file->FileSize;
	}

	return nResult;
}

//////////////////////////////////////////////////////////////////////////
//                           FileNode
//////////////////////////////////////////////////////////////////////////

FileNode::FileNode()
{
	Key = NULL;
	Parent = NULL;

	SourceName = NULL;
	SourceShortName = NULL;
	TargetName = NULL;
	TargetShortName = NULL;

	FileSize = 0;
	FileAttributes = 0;
	SequenceMark = 0;

	IsFake = false;
	FakeFileContent = NULL;

	SequentialIndex = -1;
}

FileNode::~FileNode()
{
	KILLSTR(Key);
	KILLSTR(SourceName);
	KILLSTR(SourceShortName);
	KILLSTR(TargetName);
	KILLSTR(TargetShortName);

	KILLSTR(FakeFileContent);
}

bool FileNode::Init( FileEntry *entry )
{
	if (!entry || !entry->Key || !*entry->Key)
		return false;

	Key = _wcsdup(entry->Key);

	wchar_t *colonPos = wcschr(entry->FileName, ':');
	if (colonPos)
	{
		*colonPos = 0;
		SplitEntryWithAlloc(entry->FileName, '|', TargetShortName, TargetName);
		SplitEntryWithAlloc(colonPos + 1, '|', SourceShortName, SourceName);
		*colonPos = ':';
	}
	else
	{
		SplitEntryWithAlloc(entry->FileName, '|', TargetShortName, TargetName);
	}

	FileSize = entry->FileSize;
	FileAttributes = entry->Attributes;
	SequenceMark = entry->Sequence;

	return true;
}

DWORD FileNode::GetSytemAttributes()
{
	DWORD result = 0;

	if (FileAttributes & msidbFileAttributesHidden)
		result |= FILE_ATTRIBUTE_HIDDEN;
	if (FileAttributes & msidbFileAttributesSystem)
		result |= FILE_ATTRIBUTE_SYSTEM;
	if (FileAttributes & msidbFileAttributesReadOnly)
		result |= FILE_ATTRIBUTE_READONLY;

	if (result == 0)
		result = FILE_ATTRIBUTE_NORMAL;

	return result;
}

std::wstring FileNode::GetSourcePath()
{
	wstring strResult = Parent->GetSourcePath();
	strResult.append(L"\\");
	if (SourceName)
		strResult.append(SourceName);
	else
		strResult.append(TargetName);
	return strResult;
}

std::wstring FileNode::GetTargetPath()
{
	wstring strResult = Parent->GetTargetPath();
	if (strResult.length() > 0)	strResult.append(L"\\");
	strResult.append(TargetName);

	return strResult;
}
