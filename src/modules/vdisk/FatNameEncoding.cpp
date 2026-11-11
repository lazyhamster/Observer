#include "stdafx.h"
#include "FatNameEncoding.h"
#include "FatLongNameReader.h"

using namespace System;
using namespace System::Text;

bool FatNameEncoding::IsPlainByte(Byte value)
{
	return value < 0x80 && !(value >= 'a' && value <= 'z');
}

bool FatNameEncoding::IsRawChar(Char value)
{
	return value >= RawCharBase && value < RawCharBase + 256;
}

FatNameEncoding::FatNameEncoding(Encoding^ displayEncoding)
	: m_displayEncoding(displayEncoding)
{
	//
}

bool FatNameEncoding::IsSingleByte::get()
{
	return true;
}

int FatNameEncoding::GetByteCount(array<Char>^ chars, int index, int count)
{
	return count;
}

int FatNameEncoding::GetBytes(array<Char>^ chars, int charIndex, int charCount, array<Byte>^ bytes, int byteIndex)
{
	for (int i = 0; i < charCount; i++)
	{
		Char ch = chars[charIndex + i];
		bytes[byteIndex + i] = IsRawChar(ch) ? (Byte)(ch - RawCharBase) : ((ch < 0x80) ? (Byte)ch : (Byte)'?');
	}
	return charCount;
}

int FatNameEncoding::GetCharCount(array<Byte>^ bytes, int index, int count)
{
	return count;
}

int FatNameEncoding::GetChars(array<Byte>^ bytes, int byteIndex, int byteCount, array<Char>^ chars, int charIndex)
{
	for (int i = 0; i < byteCount; i++)
	{
		Byte value = bytes[byteIndex + i];
		chars[charIndex + i] = IsPlainByte(value) ? (Char)value : (Char)(RawCharBase + value);
	}
	return byteCount;
}

int FatNameEncoding::GetMaxByteCount(int charCount)
{
	return charCount;
}

int FatNameEncoding::GetMaxCharCount(int byteCount)
{
	return byteCount;
}

void FatNameEncoding::LoadLongNames(IO::Stream^ volume)
{
	m_longNames = ReadFatLongNames(volume, this);
}

String^ FatNameEncoding::ToDisplayPath(String^ path, bool useLongNames)
{
	array<String^>^ elements = path->Split(L'\\');
	String^ shortPath = String::Empty;

	for (int i = 0; i < elements->Length; i++)
	{
		if (elements[i]->Length == 0) continue;

		shortPath = (shortPath->Length > 0) ? shortPath + "\\" + elements[i] : elements[i];

		String^ longName;
		if (useLongNames && m_longNames != nullptr && m_longNames->TryGetValue(shortPath, longName))
		{
			elements[i] = longName;
		}
		else
		{
			array<Byte>^ rawName = this->GetBytes(elements[i]);
			elements[i] = m_displayEncoding->GetString(rawName);
		}
	}
	return String::Join("\\", elements);
}
