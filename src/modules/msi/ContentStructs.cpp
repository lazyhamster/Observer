#include "StdAfx.h"
#include "ContentStructs.h"

#include <MsiDefs.h>

static void SplitEntryName(const std::wstring& source, wchar_t delim, std::wstring& shortName, std::wstring& longName)
{
	auto delimPos = source.find(delim);

	if (delimPos != std::wstring::npos)
	{
		longName = source.substr(delimPos + 1);
		shortName = (delimPos < MAX_SHORT_NAME_LEN) ? source.substr(0, delimPos) : L"";
	}
	else
	{
		// Short name can not be more then 14 symbols
		shortName = (source.size() < MAX_SHORT_NAME_LEN) ? source : L"";
		longName = source;
	}
}

//////////////////////////////////////////////////////////////////////////
//                           BasicNode
//////////////////////////////////////////////////////////////////////////

BasicNode::BasicNode()
{
	Parent = nullptr;
	Attributes = 0;

	ftCreationTime = ZERO_FILE_TIME;
	ftModificationTime = ZERO_FILE_TIME;
}

//////////////////////////////////////////////////////////////////////////
//                           DirectoryNode
//////////////////////////////////////////////////////////////////////////

DirectoryNode::DirectoryNode()
{
	Parent = nullptr;
	IsSpecial = false;

	Attributes = FILE_ATTRIBUTE_DIRECTORY;
}

DirectoryNode::~DirectoryNode()
{
	for (size_t i = 0; i < Files.size(); i++)
		delete Files[i];
	for (size_t i = 0; i < SubDirs.size(); i++)
		delete SubDirs[i];
}

bool DirectoryNode::Init(DirectoryEntry *entry, bool substDotTargetWithSource)
{
	if (!entry || entry->Key.empty())
		return false;

	Key = entry->Key;
	if (entry->ParentKey[0])
		ParentKey = entry->ParentKey;
	
	auto colonPos = entry->DefaultDir.find(':');
	if (colonPos != std::wstring::npos)
	{
		auto strSourcePart = entry->DefaultDir.substr(colonPos + 1);
		auto strTargetPath = entry->DefaultDir.substr(0, colonPos);
		
		if (substDotTargetWithSource && (strTargetPath == L".") && (strSourcePart != L"."))
		{
			// This is for special folders when using . as target path is undesirable
			// If target is . but source is not then use source in both name pairs
			SplitEntryName(strSourcePart, '|', TargetShortName, TargetName);
			SplitEntryName(strSourcePart, '|', SourceShortName, SourceName);
		}
		else
		{
			SplitEntryName(strTargetPath, '|', TargetShortName, TargetName);
			SplitEntryName(strSourcePart, '|', SourceShortName, SourceName);
		}
	}
	else
	{
		SplitEntryName(entry->DefaultDir, '|', TargetShortName, TargetName);
	}
	
	return true;
}

bool DirectoryNode::Init( const std::wstring& dirName )
{
	if (dirName.empty())
		return false;

	Key = dirName;
	ParentKey = L"";
	SourceName = dirName;
	TargetName = dirName;
	
	return true;
}

void DirectoryNode::AddSubdir(DirectoryNode *child)
{
	// Avoid duplication when adding child
	if (std::find(SubDirs.begin(), SubDirs.end(), child) == SubDirs.end())
	{
		SubDirs.push_back(child);
		child->Parent = this;
	}
}

int DirectoryNode::GetFilesCount()
{
	int nResult = (int)Files.size();
	for (size_t i = 0; i < SubDirs.size(); i++)
	{
		DirectoryNode* subdir = SubDirs[i];
		nResult += subdir->GetFilesCount();
	}
	return nResult;
}

int DirectoryNode::GetSubDirsCount()
{
	int nResult = (int)SubDirs.size();
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
	std::wstring strResult;
	if (Parent)
	{
		strResult = Parent->GetSourcePath();
		if (strResult.size() > 0)
			strResult.append(L"\\");
	}
	if (!SourceName.empty())
		strResult.append(SourceName);
	else if (!TargetName.empty() && (Key != L"TARGETDIR"))
		strResult.append(TargetName);

	return strResult;
}

std::wstring DirectoryNode::GetTargetPath()
{
	std::wstring strResult;
	if (Parent)
	{
		strResult = Parent->GetTargetPath();
		if (strResult.size() > 0)
			strResult.append(L"\\");
	}
	if (!TargetName.empty() /*&& wcscmp(Key, L"TARGETDIR")*/)
		strResult.append(TargetName);

	return strResult;
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
	Parent = nullptr;

	FileSize = 0;
	Attributes = 0;
	SequenceMark = 0;

	IsFake = false;
	FakeFileContent = NULL;
}

FileNode::~FileNode()
{
	if (FakeFileContent)
		free(FakeFileContent);
}

bool FileNode::Init( FileEntry *entry )
{
	if (!entry || entry->Key.empty())
		return false;

	Key = entry->Key;

	auto colonPos = entry->FileName.find(':');
	if (colonPos != std::wstring::npos)
	{
		SplitEntryName(entry->FileName.substr(0, colonPos), '|', TargetShortName, TargetName);
		SplitEntryName(entry->FileName.substr(colonPos + 1), '|', SourceShortName, SourceName);
	}
	else
	{
		SplitEntryName(entry->FileName, '|', TargetShortName, TargetName);
	}

	FileSize = entry->FileSize;
	Attributes = entry->Attributes;
	SequenceMark = entry->Sequence;
	Component = entry->Component_;

	return true;
}

DWORD FileNode::GetSytemAttributes()
{
	DWORD result = 0;

	if (Attributes & msidbFileAttributesHidden)
		result |= FILE_ATTRIBUTE_HIDDEN;
	if (Attributes & msidbFileAttributesSystem)
		result |= FILE_ATTRIBUTE_SYSTEM;
	if (Attributes & msidbFileAttributesReadOnly)
		result |= FILE_ATTRIBUTE_READONLY;

	if (result == 0)
		result = FILE_ATTRIBUTE_NORMAL;

	return result;
}

std::wstring FileNode::GetSourcePath()
{
	auto strResult = Parent->GetSourcePath();
	strResult.append(L"\\");
	if (!SourceName.empty())
		strResult.append(SourceName);
	else
		strResult.append(TargetName);
	return strResult;
}

std::wstring FileNode::GetTargetPath()
{
	auto strResult = Parent->GetTargetPath();
	if (strResult.length() > 0)	strResult.append(L"\\");
	strResult.append(TargetName);

	return strResult;
}
