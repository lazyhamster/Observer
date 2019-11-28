#include "stdafx.h"
#include "CommonFunc.h"

#include "InterfaceCommon.h"

std::wstring GetFinalExtractionPath(const StorageObject* storage, const ContentTreeNode* item, const wchar_t* baseDir, int keepPathOpt)
{
	std::wstring strResult(baseDir);
	IncludeTrailingPathDelim(strResult);

	ContentTreeNode* pSubRoot = NULL;
	if (keepPathOpt == KPV_PARTIAL)
		pSubRoot = storage->CurrentDir();
	else if (keepPathOpt == KPV_NOPATH)
		pSubRoot = item->parent;

	size_t nSubPathSize = item->GetPath(NULL, 0, pSubRoot) + 1;
	wchar_t *wszItemSubPath = (wchar_t *) malloc(nSubPathSize * sizeof(wchar_t));
	item->GetPath(wszItemSubPath, nSubPathSize, pSubRoot);

	strResult.append(wszItemSubPath);
	free(wszItemSubPath);

	return strResult;
}

// Returns total number of items added
int CollectFileList(ContentTreeNode* node, ContentNodeList &targetlist, __int64 &totalSize, bool recursive)
{
	int numItems = 0;

	if (node->IsDir())
	{
		if (recursive)
		{
			// Iterate through sub directories
			for (auto cit = node->subdirs.begin(); cit != node->subdirs.end(); cit++)
			{
				ContentTreeNode* child = cit->second;
				numItems += CollectFileList(child, targetlist, totalSize, recursive);
			} //for
		}

		// Iterate through files
		for (auto cit = node->files.begin(); cit != node->files.end(); cit++)
		{
			ContentTreeNode* child = cit->second;
			targetlist.push_back(child);
			numItems++;
			totalSize += child->GetSize();
		} //for

		// If it is empty directory then just add it
		if (node->GetChildCount() == 0)
			targetlist.push_back(node);
	}
	else
	{
		targetlist.push_back(node);
		numItems++;
		totalSize += node->GetSize();
	}

	return numItems;
}

std::wstring ProgressBarString(intptr_t Percentage, intptr_t Width)
{
	std::wstring result;
	// 0xB0 - 0x2591
	// 0xDB - 0x2588
	result = std::wstring((Width - 5) * (Percentage > 100 ? 100 : Percentage) / 100, 0x2588);
	result += std::wstring((Width - 5) - result.length(), 0x2591);
	result += FormatString(L"%4d%%", Percentage > 100 ? 100 : Percentage);
	return result;
}

std::wstring DurationToString(long long durationMs)
{
	long long secs = durationMs / 1000;
	long long mins = secs / 60;
	long long hours = mins / 60;

	return FormatString(L"%02lld:%02lld:%02lld", hours, mins - hours * 60, secs - mins * 60 - hours * 60 * 60);
}

std::wstring JoinProgressLine(const std::wstring &prefix, const std::wstring &suffix, size_t maxWidth, size_t rightPadding)
{
	size_t middlePadding = maxWidth - prefix.length() - suffix.length() - rightPadding;
	std::wstring result = prefix + std::wstring(middlePadding, ' ') + suffix + std::wstring(rightPadding, ' ');

	return result;
}
