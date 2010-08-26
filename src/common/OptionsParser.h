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
	
	bool GetValueAsBool(const wchar_t* Key, bool Default);
	int GetValueAsInt(const wchar_t* Key, int Default);
	wstring GetValueAsString(const wchar_t* Key, const wchar_t* Default);

	const OptionsItem& operator[](size_t index) { return m_vValues.at(index); }
	size_t NumOptions() { return m_vValues.size(); }
};

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, OptionsList &opts);

#endif // OptionsParser_h__