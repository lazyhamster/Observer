#include "stdafx.h"
#include "zlib.h"
#include "Unpacker.h"

#define BSIZE 16384

class IAbstractWriter
{
public:	
	virtual bool SaveData(const unsigned char* buf, int bufSize) = 0;
};

class FileWriter : public IAbstractWriter
{
private:
	HANDLE hFile;
public:	
	FileWriter(const wchar_t* filePath)
	{
		if (filePath && *filePath)
			hFile = CreateFile(filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		else
			hFile = INVALID_HANDLE_VALUE;
	}
	~FileWriter()
	{
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
	}
	
	bool SaveData(const unsigned char* buf, int bufSize)
	{
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwWritten;
			return WriteFile(hFile, buf, bufSize, &dwWritten, NULL) && (dwWritten == bufSize);

		}
		return true;
	}
};

class MemoryWriter : public IAbstractWriter
{
private:
	char* pMem;
	size_t nMemSize;
	size_t nDataSize;
public:	
	MemoryWriter()
	{
		nMemSize = BSIZE;
		pMem = (char*) malloc(nMemSize);
		nDataSize = 0;
	}
	~MemoryWriter()
	{
		free(pMem);
	}

	const char* GetData() { return pMem; }
	size_t GetDataSize() { return nDataSize; }

	bool SaveData(const unsigned char* buf, int bufSize)
	{
		// Expend buffer
		if (nDataSize + bufSize >= nMemSize)
		{
			while (nDataSize + bufSize >= nMemSize)
				nMemSize += BSIZE;
			
			if (nMemSize > 512 * 1024) return false;
			pMem = (char*) realloc(pMem, nMemSize);
		}
		memcpy(pMem + nDataSize, buf, bufSize);
		nDataSize += bufSize;

		return true;
	}
};

bool inflateData(HANDLE inFile, IAbstractWriter* writer, InflatedDataInfo &info)
{
	int ret;
	unsigned int have;
	unsigned char buf_in[BSIZE] = {0};
	unsigned char buf_out[BSIZE] = {0};
	DWORD dwRead;

	z_stream strm = {0};
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	ret = inflateInit2(&strm, -15);
	if (ret != Z_OK) return false;

	// Remember position in input file	
	LARGE_INTEGER nInPos = {0};
	nInPos.LowPart = SetFilePointer(inFile, 0, &nInPos.HighPart, FILE_CURRENT);

	info.crc = crc32(0L, Z_NULL, 0);
	info.packedSize = 0;
	info.unpackedSize = 0;

	/* decompress until deflate stream ends or end of file */
	do
	{
		if (!ReadFile(inFile, buf_in, BSIZE, &dwRead, NULL) || (dwRead == 0))
		{
			ret = Z_ERRNO;
			break;
		}

		strm.next_in = buf_in;
		strm.avail_in = dwRead;
		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = BSIZE;
			strm.next_out = buf_out;

			ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_NEED_DICT)
				ret = Z_DATA_ERROR;
			if (ret < 0) break;

			have = BSIZE - strm.avail_out;
			if (have == 0) break;

			// Write extracted data to output file
			if (writer != NULL && !writer->SaveData(buf_out, have))
			{
				ret = Z_ERRNO;
				break;
			}
			info.crc = crc32(info.crc, buf_out, have);
			info.unpackedSize += have;

		} while (strm.avail_out == 0);

		info.packedSize += (dwRead - strm.avail_in);

		/* done when inflate() says it's done (or error) */
	} while (ret != Z_STREAM_END && ret != Z_ERRNO);

	/* clean up and return */
	inflateEnd(&strm);

	// Position file pointer at the end of packed data
	nInPos.QuadPart += info.packedSize;
	SetFilePointer(inFile, nInPos.LowPart, &nInPos.HighPart, FILE_BEGIN);

	return (ret == Z_STREAM_END);
}

bool inflateData(HANDLE inFile, char* &memBuf, size_t &memBufSize, InflatedDataInfo &info)
{
	MemoryWriter* writer = new MemoryWriter();
	bool rval = inflateData(inFile, writer, info);

	memBufSize = writer->GetDataSize();
	if (memBufSize > 0)
	{
		memBuf = (char *) malloc(memBufSize);
		memcpy(memBuf, writer->GetData(), memBufSize);
	}

	delete writer;
	return rval;
}

bool inflateData(HANDLE inFile, const wchar_t* outFileName, InflatedDataInfo &info)
{
	FileWriter* writer = (outFileName && *outFileName) ? new FileWriter(outFileName) : NULL;
	bool rval = inflateData(inFile, writer, info);

	if (writer) delete writer;
	return rval;
}
