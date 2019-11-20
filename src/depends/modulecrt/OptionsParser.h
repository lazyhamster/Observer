#ifndef OptionsParser_h__
#define OptionsParser_h__

#include <vector>

#define OPT_KEY_MAXLEN 32
#define OPT_VAL_MAXLEN 128

struct OptionsItem
{
	wchar_t key[OPT_KEY_MAXLEN];
	wchar_t value[OPT_VAL_MAXLEN];
};

class OptionsList
{
private:
	std::vector<OptionsItem> m_vValues;

	bool AddOption(const wchar_t* Key, const wchar_t* Value);

public:
	OptionsList() {}
	OptionsList(const wchar_t* Input) { ParseLines(Input); }
	
	int ParseLines(const wchar_t* Input); // Array of strings, separated with \0
	
	bool GetValue(const wchar_t* Key, bool &Value) const;
	bool GetValue(const wchar_t* Key, int &Value) const;
	bool GetValue(const wchar_t* Key, wchar_t *Value, size_t MaxValueSize) const;

	OptionsItem GetOption(size_t index) { return m_vValues.at(index); }
	size_t NumOptions() const { return m_vValues.size(); }
};

#endif // OptionsParser_h__