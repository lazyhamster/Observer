#ifndef OptionsFile_h__
#define OptionsFile_h__

#include "modulecrt/OptionsParser.h"

class OptionsFile
{
private:
	wchar_t* m_wszFilePath;

	// Make class non-copyable
	OptionsFile(const OptionsFile &other) {}
	OptionsFile& operator =(const OptionsFile &other) {}

public:
	OptionsFile(const wchar_t* ConfigLocation);
	~OptionsFile();

	OptionsList* GetSection(const wchar_t* SectionName);
	bool GetSectionLines(const wchar_t* SectionName, wchar_t* Buffer, size_t BufferSize);
};

#endif // OptionsFile_h__