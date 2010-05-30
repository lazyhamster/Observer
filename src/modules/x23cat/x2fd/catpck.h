#ifndef CATPCK_INCLUDED
#define CATPCK_INCLUDED

#include "common/gzip.h"
#include "x2fd.h"

#include "file_io.h"

void DecryptCAT(unsigned char *buffer, const io64::file::size& size);
void DecryptDAT(unsigned char *buffer, const io64::file::size&size);
int GetFileCompressionType(const char *pszName);
int GetBufferCompressionType(const unsigned char *data, const io64::file::size& size);
int DecompressFile(io64::file& file, int compressionMethod, unsigned char **out_data, io64::file::size *out_size, time_t *mtime);
int DecompressBuffer(unsigned char *in_data, const io64::file::size& in_size, int compressionMethod, unsigned char **out_data, io64::file::size *out_size, time_t *mtime);

#endif // !defined(CATPCK_INCLUDED)