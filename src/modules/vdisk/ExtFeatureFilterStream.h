#pragma once

// DiscUtils refuses to open ext volumes with any incompatible feature flag it does not know.
// Some of them don't change how files and directories are stored, so they are safe for reading,
// e.g. metadata_csum_seed (enabled by default in recent mkfs.ext4) only changes how checksums are seeded.
// This read-only stream hides such flags in the primary superblock, so DiscUtils can open the volume.
ref class ExtFeatureFilterStream : public System::IO::Stream
{
private:
	literal __int64 FeatureIncompatOffset = 1024 + 0x60; // s_feature_incompat in primary superblock
	literal System::UInt32 IgnoredIncompatFeatures = 0x2000; // INCOMPAT_CSUM_SEED

	System::IO::Stream^ m_inner;

public:
	ExtFeatureFilterStream(System::IO::Stream^ inner);

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
