#ifndef OptionsParser_h__
#define OptionsParser_h__

#define OPT_KEY_MAXLEN 32
#define OPT_VAL_MAXLEN 64

struct OptionsItem
{
	wchar_t key[OPT_KEY_MAXLEN];
	wchar_t value[OPT_VAL_MAXLEN];
};
typedef vector<OptionsItem> OptionsList;

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, OptionsList &opts);

#endif // OptionsParser_h__