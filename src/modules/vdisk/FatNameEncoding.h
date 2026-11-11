#pragma once

// DiscUtils finds FAT items by upper-casing the path and encoding it back to bytes.
// This fails for names which do not survive that conversion (lowercase letters or text in another codepage),
// e.g. byte 0xED is 'phi' in CP437, its uppercase form is byte 0xE8, so the directory can't be found by its own path.
// To avoid this we hand DiscUtils a 1:1 byte mapping without case, and decode real names only for display.
// As a bonus, it allows multi-byte codepages, which DiscUtils rejects for FAT.
ref class FatNameEncoding : public System::Text::Encoding
{
private:
	literal int RawCharBase = 0xF700; // Private Use Area, these characters have no case mapping

	System::Text::Encoding^ m_displayEncoding;
	System::Collections::Generic::Dictionary<System::String^, System::String^>^ m_longNames;

	static bool IsPlainByte(System::Byte value);
	static bool IsRawChar(System::Char value);

public:
	FatNameEncoding(System::Text::Encoding^ displayEncoding);

	// DiscUtils refuses to use FAT encodings which are not single byte
	virtual property bool IsSingleByte
	{
		bool get() override;
	}

	virtual int GetByteCount(array<System::Char>^ chars, int index, int count) override;
	virtual int GetBytes(array<System::Char>^ chars, int charIndex, int charCount, array<System::Byte>^ bytes, int byteIndex) override;
	virtual int GetCharCount(array<System::Byte>^ bytes, int index, int count) override;
	virtual int GetChars(array<System::Byte>^ bytes, int byteIndex, int byteCount, array<System::Char>^ chars, int charIndex) override;
	virtual int GetMaxByteCount(int charCount) override;
	virtual int GetMaxCharCount(int byteCount) override;

	void LoadLongNames(System::IO::Stream^ volume);

	// Converts internal path to a readable one. Works per path element, as they are separate byte sequences.
	System::String^ ToDisplayPath(System::String^ path, bool useLongNames);
};
