#ifndef OptionsParser_h__
#define OptionsParser_h__

struct OptionsItem
{
	wstring key;
	wstring value;
};
typedef vector<OptionsItem> OptionsList;

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, OptionsList &opts);

#endif // OptionsParser_h__