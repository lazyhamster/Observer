#ifndef GZIP_WRAPPER_INCLUDED
#define GZIP_WRAPPER_INCLUDED

#include <time.h>
#include <zlib.h>
#include "strutils.h"

#define GZ_COMP_MAX 9

#define GZ_E_OK               0   // ok
#define GZ_E_FLAGS            1   // reserved bits are set
#define GZ_E_HEADER         (-1)  // no gzip header
#define GZ_E_TOOSMALL       (-2)  // input stream is too small
#define GZ_E_CRC            (-3)  // data crc does not match
#define GZ_E_HCRC           (-4)  // header crc does not match
#define GZ_E_COMPRESSION    (-5)  // unsupported compression
#define GZ_E_DEFLATE        (-6)  // deflate failed
#define GZ_E_FILE           (-7)  // error opening file
#define GZ_E_INTERNAL_ERROR (-8)  // access violation

#define GZ_CM_DEFLATE 8

#define GZ_F_TEXT     1     // 0
#define GZ_F_HCRC     2     // 1
#define GZ_F_EXTRA    4     // 2
#define GZ_F_FILENAME 8     // 3
#define GZ_F_COMMENT  16    // 4
#define GZ_F_RES1     32    // 5
#define GZ_F_RES2     64    // 6
#define GZ_F_RES3     128   // 7

#define GZ_ID1 0x1F
#define GZ_ID2 0x8B

#define GZ_CM     2
#define GZ_FLG    3
#define GZ_MTIME  4
#define GZ_XFL    8
#define GZ_OS     9

class gzreader
{
  private:
    static const size_t header_size=10;

    size_t *m_unpackedsize;
    size_t *m_crc;
    unsigned short int *m_hcrc;
    size_t *m_xlen;
    char * m_origname;
    char * m_comment;
    time_t m_mtime;

    unsigned char *m_stream;  // packed data
    size_t m_streamsize;

    unsigned char *m_buffer;      // buffer
    unsigned char *m_fileBuffer;  // file buffer
    unsigned char *m_outdata;     // decompressed data
    size_t m_outsize; // number of decoded bytes

    int m_error;

  public:
    typedef unsigned char value_type;

    gzreader() {
      // absolutely unsafe - but I don't use virtual methods nor do I derive
      // from this class so I can use it
      memset(this, 0, sizeof(*this));
    }

    ~gzreader() { close(); }

    int error() const { return m_error; }
    void error(int err) { m_error=err; }

    value_type compression() const { return m_buffer[GZ_CM]; }
    value_type flags() const { return m_buffer[GZ_FLG]; }
    value_type xflags() const { return m_buffer[GZ_XFL]; }
    value_type osystem() const { return m_buffer[GZ_OS]; }
    char * origFileName() const { return m_origname; }
    char * comment() const { return m_comment; }
    time_t mtime() const { return m_mtime; }
    size_t crc() const { return *m_crc; }
    size_t hcrc() const { return (m_hcrc ? *m_hcrc : 0); }
    size_t streamsize() const { return m_streamsize; }
    size_t unpackedsize() const { return *m_unpackedsize; }

    value_type * buffer() const { return m_buffer; }
    value_type * stream() const { return m_stream; }
    value_type * outdata() const { return m_outdata; }
    size_t outsize() const { return m_outsize; }

    bool openFile(const char *pszName);
    void close()
    {
      if(m_fileBuffer) delete[] m_fileBuffer; m_fileBuffer=NULL;
      m_buffer=NULL; m_streamsize=0; m_origname=NULL; m_comment=NULL;
      m_unpackedsize=NULL;
      m_hcrc=NULL; m_hcrc=NULL; m_xlen=NULL;
    }

    void flush() { if(m_outdata) delete[] m_outdata; m_outdata=NULL;
    m_outsize=0; }
    bool unpack(value_type *buffer, size_t size);
};

#endif // !defined(GZIP_WRAPPER_INCLUDED)
