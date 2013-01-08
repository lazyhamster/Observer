#ifndef Config_h__
#define Config_h__

typedef std::vector<std::wstring> StringList;

class ConfigSection
{
public:
	ConfigSection(const wchar_t* Name);
	~ConfigSection();

	const wchar_t* GetName() const;

	StringList GetKeys() const;

	bool GetValue(const wchar_t* Key, bool &Value) const;
	bool GetValue(const wchar_t* Key, int &Value) const;
	bool GetValue(const wchar_t* Key, wchar_t *Value, size_t MaxValueSize) const;
	bool GetValue(const wchar_t* Key, char *Value, size_t MaxValueSize) const;
};

class Config
{
public:
	Config();
	~Config();

	bool ParseFile(const wchar_t* path);

	bool GetValue(const wchar_t* Section, const wchar_t* Key, bool &Value) const;
	bool GetValue(const wchar_t* Section, const wchar_t* Key, int &Value) const;
	bool GetValue(const wchar_t* Section, const wchar_t* Key, wchar_t *Value, size_t MaxValueSize) const;
	bool GetValue(const wchar_t* Section, const wchar_t* Key, char *Value, size_t MaxValueSize) const;
};

#endif // Config_h__
