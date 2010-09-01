#ifndef OptionsParser_h__
#define OptionsParser_h__

#define OPT_KEY_MAXLEN 32
#define OPT_VAL_MAXLEN 64

struct OptionsItem
{
	wchar_t key[OPT_KEY_MAXLEN];
	wchar_t value[OPT_VAL_MAXLEN];
};

class OptionsList
{
private:
	vector<OptionsItem> m_vValues;

	bool AddOption(const wchar_t* Key, const wchar_t* Value);

public:
	int ParseLines(const wchar_t* Input); // Array of strings, separated with \0
	
	bool GetValue(const wchar_t* Key, bool &Value) const;
	bool GetValue(const wchar_t* Key, int &Value) const;
	bool GetValue(const wchar_t* Key, wchar_t *Value, size_t MaxValueSize) const;
	bool GetValue(const wchar_t* Key, char *Value, size_t MaxValueSize) const;

	OptionsItem GetOption(size_t index) { return m_vValues.at(index); }
	size_t NumOptions() const { return m_vValues.size(); }
};

class OptionsFile
{
private:
	wchar_t m_wszFilePath[MAX_PATH];
	
	// Make class non-copyable
	OptionsFile(const OptionsFile &other) {}
	OptionsFile& operator =(const OptionsFile &other) {}

public:
	OptionsFile(const wchar_t* ConfigLocation);

	OptionsList* GetSection(const wchar_t* SectionName);
	bool GetSectionLines(const wchar_t* SectionName, wchar_t* Buffer, size_t BufferSize);
};

#endif // OptionsParser_h__