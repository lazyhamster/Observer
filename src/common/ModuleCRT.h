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

static inline void RenameInvalidPathChars(wchar_t* input)
{
	size_t inputLen = wcslen(input);
	for (size_t i = 0; i < inputLen; i++)
	{
		if (!IsValidPathChar(input[i]))
			input[i] = '_';
	}
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

#endif // ModuleCRT_h__