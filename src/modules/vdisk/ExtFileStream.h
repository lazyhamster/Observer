#pragma once

// Read-only content of an ext4 file that uses extents.
// DiscUtils hangs when reading files with holes (unallocated ranges which read as zeros),
// because it doesn't handle blocks between extents or after the last one.
// This stream maps file blocks itself, holes and unwritten extents are returned as zeros.
ref class ExtFileStream : public System::IO::Stream
{
public:
	value struct Run
	{
		System::UInt32 LogicalBlock;
		System::UInt32 BlockCount;
		System::UInt64 PhysicalBlock;
		bool Initialized;
	};

private:
	System::IO::Stream^ m_volume;
	int m_blockSize;
	__int64 m_length;
	__int64 m_position;
	array<Run>^ m_runs; // Sorted by LogicalBlock

	int FindRun(System::UInt64 block);

public:
	ExtFileStream(System::IO::Stream^ volume, int blockSize, __int64 length, System::Collections::Generic::List<Run>^ runs);

	// Reads leaf entries of extent tree node at volume block, appends them to runs
	static void ReadExtentNode(System::IO::Stream^ volume, int blockSize, System::UInt64 block, int depthLeft, System::Collections::Generic::List<Run>^ runs);

	virtual property bool CanRead { bool get() override; }
	virtual property bool CanSeek { bool get() override; }
	virtual property bool CanWrite { bool get() override; }
	virtual property __int64 Length { __int64 get() override; }
	virtual property __int64 Position { __int64 get() override; void set(__int64 value) override; }

	virtual void Flush() override;
	virtual int Read(array<System::Byte>^ buffer, int offset, int count) override;
	virtual __int64 Seek(__int64 offset, System::IO::SeekOrigin origin) override;
	virtual void SetLength(__int64 value) override;
	virtual void Write(array<System::Byte>^ buffer, int offset, int count) override;
};
