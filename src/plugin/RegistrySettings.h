#ifndef RegistrySettings_h__
#define RegistrySettings_h__

class RegistrySettings
{
private:
	wstring m_strRegKeyName;
	HKEY m_hkRegKey;
	bool m_fCanWrite;

	// Make class non-copyable
	RegistrySettings(const RegistrySettings &other) {}
	RegistrySettings& operator =(const RegistrySettings &other) {}

public:
	RegistrySettings(const wchar_t* RootKey);
	RegistrySettings(const char* RootKey);
	~RegistrySettings();

	bool Open(int CanWrite = false);

	bool GetValue(const wchar_t* ValueName, int &Output);
	bool GetValue(const wchar_t* ValueName, wchar_t *Output, size_t OutputMaxSize);
	bool GetValue(const char* ValueName, char *Output, size_t OutputMaxSize);

	bool SetValue(const wchar_t* ValueName, int Value);
	bool SetValue(const wchar_t* ValueName, const wchar_t *Value);
	bool SetValue(const char* ValueName, const char *Value);
};

#endif // RegistrySettings_h__