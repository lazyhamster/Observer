#include "stdafx.h"
#include "ExtFeatureFilterStream.h"

using namespace System;
using namespace System::IO;

ExtFeatureFilterStream::ExtFeatureFilterStream(Stream^ inner)
	: m_inner(inner)
{
}

bool ExtFeatureFilterStream::CanRead::get()
{
	return true;
}

bool ExtFeatureFilterStream::CanSeek::get()
{
	return m_inner->CanSeek;
}

bool ExtFeatureFilterStream::CanWrite::get()
{
	return false;
}

__int64 ExtFeatureFilterStream::Length::get()
{
	return m_inner->Length;
}

__int64 ExtFeatureFilterStream::Position::get()
{
	return m_inner->Position;
}

void ExtFeatureFilterStream::Position::set(__int64 value)
{
	m_inner->Position = value;
}

void ExtFeatureFilterStream::Flush()
{
}

int ExtFeatureFilterStream::Read(array<Byte>^ buffer, int offset, int count)
{
	__int64 start = m_inner->Position;
	int bytesRead = m_inner->Read(buffer, offset, count);

	// Clear ignored bits in every byte of the little-endian field that was read
	for (int i = 0; i < 4; i++)
	{
		__int64 pos = FeatureIncompatOffset + i;
		if (pos >= start && pos < start + bytesRead)
		{
			Byte mask = (Byte)((IgnoredIncompatFeatures >> (i * 8)) & 0xFF);
			buffer[offset + (int)(pos - start)] &= (Byte)~mask;
		}
	}

	return bytesRead;
}

__int64 ExtFeatureFilterStream::Seek(__int64 offset, SeekOrigin origin)
{
	return m_inner->Seek(offset, origin);
}

void ExtFeatureFilterStream::SetLength(__int64 value)
{
	throw gcnew NotSupportedException();
}

void ExtFeatureFilterStream::Write(array<Byte>^ buffer, int offset, int count)
{
	throw gcnew NotSupportedException();
}
