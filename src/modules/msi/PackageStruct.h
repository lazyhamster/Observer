#ifndef _PACKAGE_STRUCT_H_
#define _PACKAGE_STRUCT_H_

// Aliases for MSI types
typedef std::wstring Identifier;
typedef std::wstring Formatted;
typedef std::wstring Text;
typedef std::wstring Version;
typedef std::wstring Language;
typedef std::wstring Filename;
typedef std::wstring DefaultDir;
typedef std::wstring Shortcut;
typedef std::wstring RegPath;
typedef int16_t Integer;
typedef int32_t DoubleInteger;

// Data structures have all fields that are present in docs.
// Ones that are not needed by reader are commented out.
// Date type names are taken from docs.

// Table: Directory
struct DirectoryEntry
{
	Identifier Key;
	Identifier ParentKey;
	DefaultDir DefaultDir;
};

// Table: Component
struct ComponentEntry
{
	Identifier Key;
	//GUID ComponentId;
	Identifier Directory_;
	Integer Attributes;
	//Condition Condition;
	//Identifier KeyPath;
};

// Table: File
struct FileEntry
{
	Identifier Key;
	Identifier Component_;
	Filename FileName;
	DoubleInteger FileSize;
	//Version	Version;
	//Language Language;
	Integer Attributes;
	Integer Sequence;
};

// Table: Feature
struct FeatureEntry
{
	Identifier Key;
	Identifier Parent;
	Text Title;
	Text Description;
	Integer Display;
	Integer Level;
	//Identifier Directory;
	//Integer Attributes;
};


// Table: Media
struct MediaEntry
{
	Integer DiskId;
	Integer LastSequence;
	//Text DiskPrompt;
	Identifier Cabinet;
	//Text VolumeLabel;
	//Property Source;
};

// Table: _Streams
struct StreamEntry
{
	wchar_t Name[63];			// s62
	char* Data;					// V0 (binary stream)
};

// Table: Registry
struct RegistryEntry
{
	Identifier Key;
	Integer Root;
	RegPath RegKeyName;
	Formatted Name;
	Formatted Value;
	Identifier Component_;
};

// Table: Shortcut
struct ShortcutEntry
{
	Identifier Key;
	Identifier Directory_;
	Filename Name;
	Identifier Component_;
	Shortcut Target;
	Formatted Arguments;
	Text Description;
	//Integer Hotkey;
	//Identifier Icon_;
	//Integer IconIndex;
	//Identifier WkDir;
	//Formatted DisplayResourceDLL;
	//Integer DisplayResourceId;
	//Formatted DescriptionResourceDLL;
	//Integer DescriptionResourceId;
};

// Table: ServiceInstall
struct ServiceInstallEntry
{
	Identifier Key;
	Formatted Name;
	Formatted DisplayName;
	DoubleInteger ServiceType;
	DoubleInteger StartType;
	//DoubleInteger ErrorControl;
	//Formatted LoadOrderGroup;
	//Formatted Dependencies;
	Formatted StartName;
	Formatted Password;
	Formatted Arguments;
	Identifier Component_;
	Formatted Description;
};

// Table: Environment
struct EnvironmentEntry
{
	Identifier Environment;
	Text Name;
	Formatted Value;
	Identifier Component_;
};

// Table: CustomAction
struct CustomActionEntry
{
	Identifier Action;
	Integer Type;
	Identifier Source;  // original type: CustomSource
	Formatted Target;
	DoubleInteger ExtendedType;
};

#endif //_PACKAGE_STRUCT_H_