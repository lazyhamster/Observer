#ifndef ModuleCRT_h__
#define ModuleCRT_h__

static inline void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

static inline bool IsValidPathChar(wchar_t ch)
{
	return (wcschr(L":*?\"<>|\\/", ch) == NULL);
}

static inline void RenameInvalidPathChars(wchar_t* input, wchar_t substChar)
{
	size_t inputLen = wcslen(input);
	for (size_t i = 0; i < inputLen; i++)
	{
		if (!IsValidPathChar(input[i]))
			input[i] = substChar;
	}
}

static inline void RenameInvalidPathChars(wchar_t* input)
{
	RenameInvalidPathChars(input, '_');
}

static __int64 GetFileSize(const wchar_t* path)
{
	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindFirstFile(path, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return ((__int64) fd.nFileSizeHigh << 32) | (__int64) fd.nFileSizeLow;
	}

	return 0;
}

static void IncludeTrailingPathDelim(wchar_t *pathBuf, size_t bufMaxSize)
{
	size_t nPathLen = wcslen(pathBuf);
	if (pathBuf[nPathLen - 1] != '\\')
		wcscat_s(pathBuf, bufMaxSize, L"\\");
}

static void TrimStr(wchar_t* str)
{
	size_t strLen = wcslen(str);
	
	// Trim at the end
	while (strLen > 0 && (str[strLen - 1] == ' ' || str[strLen - 1] == '\n' || str[strLen - 1] == '\r'))
	{
		str[strLen - 1] = 0;
		strLen--;
	}

	// Trim at the start
	while (strLen > 0 && (str[0] == ' ' || str[0] == '\n' || str[0] == '\r'))
	{
		memmove(str, str + 1, strLen - 1);
		strLen--;
	}
}

#endif // ModuleCRT_h__