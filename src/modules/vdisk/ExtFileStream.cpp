#include "stdafx.h"
#include "ExtFileStream.h"

using namespace System;
using namespace System::IO;
using namespace System::Collections::Generic;

ref class RunComparer : IComparer<ExtFileStream::Run>
{
public:
	virtual int Compare(ExtFileStream::Run a, ExtFileStream::Run b)
	{
		return a.LogicalBlock.CompareTo(b.LogicalBlock);
	}
};

ExtFileStream::ExtFileStream(Stream^ volume, int blockSize, __int64 length, List<Run>^ runs)
	: m_volume(volume), m_blockSize(blockSize), m_length(length), m_position(0)
{
	m_runs = runs->ToArray();
	Array::Sort(m_runs, gcnew RunComparer());
}

void ExtFileStream::ReadExtentNode(Stream^ volume, int blockSize, UInt64 block, int depthLeft, List<Run>^ runs)
{
	if (depthLeft <= 0)
		throw gcnew IOException("Extent tree is too deep");

	array<Byte>^ data = gcnew array<Byte>(blockSize);
	volume->Position = (__int64)(block * (UInt64)blockSize);
	int done = 0;
	while (done < blockSize)
	{
		int n = volume->Read(data, done, blockSize - done);
		if (n <= 0) throw gcnew IOException("Unable to read extent block");
		done += n;
	}

	if (BitConverter::ToUInt16(data, 0) != 0xF30A)
		throw gcnew IOException("Invalid extent block");

	int entries = BitConverter::ToUInt16(data, 2);
	int depth = BitConverter::ToUInt16(data, 6);
	if (12 + entries * 12 > blockSize)
		throw gcnew IOException("Invalid extent block");

	for (int i = 0; i < entries; i++)
	{
		int pos = 12 + i * 12;
		if (depth == 0)
		{
			// ee_block, ee_len, ee_start_hi, ee_start_lo; ee_len above 32768 means unwritten extent
			UInt16 len = BitConverter::ToUInt16(data, pos + 4);
			Run run;
			run.LogicalBlock = BitConverter::ToUInt32(data, pos);
			run.Initialized = (len <= 32768);
			run.BlockCount = run.Initialized ? len : len - 32768u;
			run.PhysicalBlock = ((UInt64)BitConverter::ToUInt16(data, pos + 6) << 32) | BitConverter::ToUInt32(data, pos + 8);
			runs->Add(run);
		}
		else
		{
			// ei_block, ei_leaf_lo, ei_leaf_hi
			UInt64 leaf = ((UInt64)BitConverter::ToUInt16(data, pos + 8) << 32) | BitConverter::ToUInt32(data, pos + 4);
			ReadExtentNode(volume, blockSize, leaf, depthLeft - 1, runs);
		}
	}
}

int ExtFileStream::FindRun(UInt64 block)
{
	// Returns index of the last run starting at or before block, -1 if none
	int lo = 0, hi = m_runs->Length - 1, found = -1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		if (m_runs[mid].LogicalBlock <= block)
		{
			found = mid;
			lo = mid + 1;
		}
		else
		{
			hi = mid - 1;
		}
	}
	return found;
}

int ExtFileStream::Read(array<Byte>^ buffer, int offset, int count)
{
	int total = 0;
	while (total < count && m_position < m_length)
	{
		UInt64 block = (UInt64)(m_position / m_blockSize);
		int blockOffset = (int)(m_position % m_blockSize);
		__int64 limit = Math::Min((__int64)(count - total), m_length - m_position);

		int idx = FindRun(block);
		bool mapped = (idx >= 0) && (block < (UInt64)m_runs[idx].LogicalBlock + m_runs[idx].BlockCount);

		int chunk;
		if (mapped && m_runs[idx].Initialized)
		{
			Run run = m_runs[idx];
			__int64 runEnd = ((__int64)run.LogicalBlock + run.BlockCount) * m_blockSize;
			chunk = (int)Math::Min(limit, runEnd - m_position);

			m_volume->Position = (__int64)((run.PhysicalBlock + (block - run.LogicalBlock)) * (UInt64)m_blockSize) + blockOffset;
			int n = m_volume->Read(buffer, offset + total, chunk);
			if (n <= 0) break;
			chunk = n;
		}
		else
		{
			// Hole or unwritten extent: zeros up to the next run or end of the current one
			__int64 zeroEnd = m_length;
			if (mapped)
				zeroEnd = ((__int64)m_runs[idx].LogicalBlock + m_runs[idx].BlockCount) * m_blockSize;
			else if (idx + 1 < m_runs->Length)
				zeroEnd = (__int64)m_runs[idx + 1].LogicalBlock * m_blockSize;

			chunk = (int)Math::Min(limit, zeroEnd - m_position);
			Array::Clear(buffer, offset + total, chunk);
		}

		total += chunk;
		m_position += chunk;
	}
	return total;
}

bool ExtFileStream::CanRead::get()
{
	return true;
}

bool ExtFileStream::CanSeek::get()
{
	return true;
}

bool ExtFileStream::CanWrite::get()
{
	return false;
}

__int64 ExtFileStream::Length::get()
{
	return m_length;
}

__int64 ExtFileStream::Position::get()
{
	return m_position;
}

void ExtFileStream::Position::set(__int64 value)
{
	if (value < 0) throw gcnew ArgumentOutOfRangeException("value");
	m_position = value;
}

void ExtFileStream::Flush()
{
}

__int64 ExtFileStream::Seek(__int64 offset, SeekOrigin origin)
{
	switch (origin)
	{
	case SeekOrigin::Current:
		offset += m_position;
		break;
	case SeekOrigin::End:
		offset += m_length;
		break;
	}
	Position = offset;
	return m_position;
}

void ExtFileStream::SetLength(__int64 value)
{
	throw gcnew NotSupportedException();
}

void ExtFileStream::Write(array<Byte>^ buffer, int offset, int count)
{
	throw gcnew NotSupportedException();
}
