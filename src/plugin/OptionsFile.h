#ifndef OptionsFile_h__
#define OptionsFile_h__

#include "OptionsParser.h"

class OptionsFile
{
private:
	wchar_t m_wszFilePath[MAX_PATH];

	// Make class non-copyable
	OptionsFile(const OptionsFile &other) {}
	OptionsFile& operator =(const OptionsFile &other) {}

public:
	OptionsFile(const wchar_t* ConfigLocation);

	OptionsList* GetSection(const wchar_t* SectionName);
	bool GetSectionLines(const wchar_t* SectionName, wchar_t* Buffer, size_t BufferSize);
};

#endif // OptionsFile_h__