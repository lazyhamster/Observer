#include "stdafx.h"

#include <mspack.h>

struct mspack_file_p {
	FILE *fh;
	const wchar_t *name;
};

static struct mspack_file *msp_open(struct mspack_system *self,
	const char *filename, int mode)
{
	struct mspack_file_p *fh;
	const wchar_t *fmode;

	switch (mode) {
	case MSPACK_SYS_OPEN_READ:   fmode = L"rb";  break;
	case MSPACK_SYS_OPEN_WRITE:  fmode = L"wb";  break;
	case MSPACK_SYS_OPEN_UPDATE: fmode = L"r+b"; break;
	case MSPACK_SYS_OPEN_APPEND: fmode = L"ab";  break;
	default: return NULL;
	}

	if ((fh = (struct mspack_file_p *) malloc(sizeof(struct mspack_file_p)))) {
		fh->name = (const wchar_t*) filename;
		if (_wfopen_s(&fh->fh, fh->name, fmode) == 0) return (struct mspack_file *) fh;
		free(fh);
	}
	return NULL;
}

static void msp_close(struct mspack_file *file) {
	struct mspack_file_p *self = (struct mspack_file_p *) file;
	if (self) {
		fclose(self->fh);
		free(self);
	}
}

static int msp_read(struct mspack_file *file, void *buffer, int bytes) {
	struct mspack_file_p *self = (struct mspack_file_p *) file;
	if (self && buffer && bytes >= 0) {
		size_t count = fread(buffer, 1, (size_t) bytes, self->fh);
		if (!ferror(self->fh)) return (int) count;
	}
	return -1;
}

static int msp_write(struct mspack_file *file, void *buffer, int bytes) {
	struct mspack_file_p *self = (struct mspack_file_p *) file;
	if (self && buffer && bytes >= 0) {
		size_t count = fwrite(buffer, 1, (size_t) bytes, self->fh);
		if (!ferror(self->fh)) return (int) count;
	}
	return -1;
}

static int msp_seek(struct mspack_file *file, off_t offset, int mode) {
	struct mspack_file_p *self = (struct mspack_file_p *) file;
	if (self) {
		switch (mode) {
		case MSPACK_SYS_SEEK_START: mode = SEEK_SET; break;
		case MSPACK_SYS_SEEK_CUR:   mode = SEEK_CUR; break;
		case MSPACK_SYS_SEEK_END:   mode = SEEK_END; break;
		default: return -1;
		}
#ifdef HAVE_FSEEKO
		return fseeko(self->fh, offset, mode);
#else
		return fseek(self->fh, offset, mode);
#endif
	}
	return -1;
}

static off_t msp_tell(struct mspack_file *file) {
	struct mspack_file_p *self = (struct mspack_file_p *) file;
#ifdef HAVE_FSEEKO
	return (self) ? (off_t) ftello(self->fh) : 0;
#else
	return (self) ? (off_t) ftell(self->fh) : 0;
#endif
}

static void msp_msg(struct mspack_file *file, const char *format, ...) {
	va_list ap;
	if (file) fprintf(stderr, "%S: ", ((struct mspack_file_p *) file)->name);
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fputc((int) '\n', stderr);
	fflush(stderr);
}

static void *msp_alloc(struct mspack_system *self, size_t bytes) {
	return malloc(bytes);
}

static void msp_free(void *buffer) {
	if (buffer)
		free(buffer);
}

static void msp_copy(void *src, void *dest, size_t bytes) {
	memcpy(dest, src, bytes);
}

static struct mspack_system msp_system = {
	&msp_open, &msp_close, &msp_read,  &msp_write, &msp_seek,
	&msp_tell, &msp_msg, &msp_alloc, &msp_free, &msp_copy, NULL
};

struct mspack_system *mspack_direct_system = &msp_system;
