#ifndef Config_h__
#define Config_h__

typedef std::vector<std::wstring> StringList;

struct ConfigItem
{
	std::wstring Key;
	std::wstring Value;

	wstring ToString() const
	{
		return Key + L"=" + Value;
	}
};

class ConfigSection
{
private:
	std::wstring m_SectionName;
	std::vector<ConfigItem> m_Items;

	bool GetValueByKey(const wchar_t* Key, wstring &FoundValue) const;

public:
	ConfigSection(const wchar_t* Name) : m_SectionName(Name) {}

	const wchar_t* GetName() const;

	StringList GetKeys() const;
	
	size_t Count() const { return m_Items.size(); }
	const ConfigItem& GetItem(size_t index) { return m_Items[index]; }
	
	void AddItem(const wchar_t* Key, const wchar_t* Value);

	bool GetValue(const wchar_t* Key, bool &Value) const;
	bool GetValue(const wchar_t* Key, int &Value) const;
	bool GetValue(const wchar_t* Key, wchar_t *Value, size_t MaxValueSize) const;
	bool GetValue(const wchar_t* Key, char *Value, size_t MaxValueSize) const;
	bool GetValue(const wchar_t* Key, std::wstring &Value) const;
	bool GetValue(const wchar_t* Key, wchar_t &Value) const;

	wstring GetAll() const;
};

class Config
{
private:
	std::map<std::wstring, ConfigSection*> m_Sections;

	int ParseSectionValues(ConfigSection* section, const wchar_t* configFilePath);

	// Make it noncopyable
	Config( const Config& );
	const Config& operator=( const Config& );

public:
	Config();
	~Config();
	
	bool ParseFile(const wchar_t* path);
	bool ParseFile(const wstring& path);

	ConfigSection* GetSection(const wchar_t* sectionName);
	ConfigSection* AddSection(const wchar_t* sectionName);
	bool IsSectionExists(const wchar_t* sectionName);
};

#endif // Config_h__
