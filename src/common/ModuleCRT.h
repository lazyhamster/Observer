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

#endif // ModuleCRT_h__