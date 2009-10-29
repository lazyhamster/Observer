#ifndef OptionsParser_h__
#define OptionsParser_h__

typedef map<wstring, wstring> WStringMap;

bool ParseOptions(const wchar_t* wszConfigFile, const wchar_t* wszSectionName, WStringMap &opts);

#endif // OptionsParser_h__