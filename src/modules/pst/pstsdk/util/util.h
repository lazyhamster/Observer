//! \file
//! \brief General utility functions and classes
//! \author Terry Mahaffey
//!
//! This is where any generalized utility classes and functions go which
//! are not directly related to MS-PST in some fashion. It is my hope that
//! this file stay as small as possible.
//! \ingroup util

#ifndef PSTSDK_UTIL_UTIL_H
#define PSTSDK_UTIL_UTIL_H

#include <cstdio>
#include <time.h>
#include <memory>
#include <vector>
#include <boost/utility.hpp>

#include "pstsdk/util/errors.h"
#include "pstsdk/util/primitives.h"

namespace pstsdk
{

//! \brief A generic class to read and write to a file
//!
//! This was necessary to get around the 32 bit limit (4GB) file size
//! limitation in ANSI C++. I needed to use compiler specific work arounds,
//! and that logic is centralized here.
//! \ingroup util
class file : private boost::noncopyable
{
public:
    //! \brief Construct a file object from the given filename
    //! \throw runtime_error if an error occurs opening the file
    //! \param[in] filename The file to open
    file(const std::wstring& filename);
    
    //! \brief Close the file
    ~file();

    //! \brief Read from the file
    //! \throw out_of_range if the requested location or location+size is past EOF
    //! \param[in,out] buffer The buffer to store the data in. The size of this vector is the amount of data to read.
    //! \param[in] offset The location on disk to read the data from.
    //! \returns The amount of data read
    size_t read(std::vector<byte>& buffer, ulonglong offset) const;

//! \cond write_api

    //! \brief Write to the file
    //! \throw out_of_range if the requested location or location+size is past EOF
    //! \param[in] buffer The data to write. The size of this vector is the amount of data to write.
    //! \param[in] offset The location on disk to read the data from.
    //! \returns The amount of data written
    size_t write(const std::vector<byte>& buffer, ulonglong offset);
//! \endcond

private:
    std::wstring m_filename;    //!< The filename used to open this file
    FILE * m_pfile;             //!< The file pointer
};


//! \brief Convert from a filetime to time_t
//!
//! FILETIME is a Win32 date/time type representing the number of 100 ns
//! intervals since Jan 1st, 1601. This function converts it to a standard
//! time_t.
//! \param[in] filetime The time to convert from
//! \returns filetime, as a time_t.
//! \ingroup util
time_t filetime_to_time_t(ulonglong filetime);

//! \brief Convert from a time_t to filetime
//!
//! FILETIME is a Win32 date/time type representing the number of 100 ns
//! intervals since Jan 1st, 1601. This function converts to it from a 
//! standard time_t.
//! \param[in] time The time to convert from
//! \returns time, as a filetime.
//! \ingroup util
ulonglong time_t_to_filetime(time_t time);

//! \brief Convert from a VT_DATE to a time_t
//!
//! VT_DATE is an OLE date/time type where the integer part is the date and
//! the fraction part is the time. This function converts it to a standard
//! time_t
//! \param[in] vt_time The time to convert from
//! \returns vt_time, as a time_t
//! \ingroup util
time_t vt_date_to_time_t(double vt_time);

//! \brief Convert from a time_t to a VT_DATE
//!
//! VT_DATE is an OLE date/time type where the integer part is the date and
//! the fraction part is the time. This function converts to it from a 
//! standard time_t.
//! \param[in] time The time to convert from
//! \returns time, as a VT_DATE
//! \ingroup util
double time_t_to_vt_date(time_t time);

//! \brief Test to see if the specified bit in the buffer is set
//! \param[in] pbytes The buffer to check
//! \param[in] bit Which bit to test
//! \returns true if the specified bit is set, false if not
//! \ingroup util
bool test_bit(const byte* pbytes, ulong bit);

//! \brief Convert an array of bytes to a std::wstring
//! \param[in] bytes The bytes to convert
//! \returns A std::wstring
//! \ingroup util
std::wstring bytes_to_wstring(const std::vector<byte> &bytes);

//! \brief Convert a std::wstring to an array of bytes
//! \param[in] wstr The std::wstring to convert
//! \returns An array of bytes
//! \ingroup util
std::vector<byte> wstring_to_bytes(const std::wstring &wstr);

} // end pstsdk namespace

inline pstsdk::file::file(const std::wstring& filename)
: m_filename(filename)
{
    const char* mode = "rb";

#ifdef _MSC_VER 
    errno_t err = fopen_s(&m_pfile, std::string(filename.begin(), filename.end()).c_str(), mode);
    if(err != 0)
        m_pfile = NULL;
#else
    m_pfile = fopen(std::string(filename.begin(), filename.end()).c_str(), mode);
#endif
    if(m_pfile == NULL)
        throw std::runtime_error("fopen failed");
}

inline pstsdk::file::~file()
{
    fflush(m_pfile);
    fclose(m_pfile);
}

inline size_t pstsdk::file::read(std::vector<byte>& buffer, ulonglong offset) const
{
#ifdef _MSC_VER
    if(_fseeki64(m_pfile, offset, SEEK_SET) != 0)
#else
    if(fseek(m_pfile, offset, SEEK_SET) != 0)
#endif
    {
        throw std::out_of_range("fseek failed");
    }

    size_t read = fread(&buffer[0], 1, buffer.size(), m_pfile);

    if(read != buffer.size())
        throw std::out_of_range("fread failed");

    return read;
}

//! \cond write_api
inline size_t pstsdk::file::write(const std::vector<byte>& buffer, ulonglong offset)
{
#ifdef _MSC_VER
    if(_fseeki64(m_pfile, offset, SEEK_SET) != 0)
#else
    if(fseek(m_pfile, offset, SEEK_SET) != 0)
#endif
    {
        throw std::out_of_range("fseek failed");
    }

    size_t write = fwrite(&buffer[0], 1, buffer.size(), m_pfile);

    if(write != buffer.size())
        throw std::out_of_range("fwrite failed");

    return write;
}
//! \endcond

inline time_t pstsdk::filetime_to_time_t(ulonglong filetime)
{
    const ulonglong jan1970 = 116444736000000000ULL;

    return (filetime - jan1970) / 10000000;
}

inline pstsdk::ulonglong pstsdk::time_t_to_filetime(time_t time)
{
    const ulonglong jan1970 = 116444736000000000ULL;

    return (time * 10000000) + jan1970;
}

inline time_t pstsdk::vt_date_to_time_t(double)
{
    throw not_implemented("vt_date_to_time_t");
}

inline double pstsdk::time_t_to_vt_date(time_t)
{
    throw not_implemented("vt_date_to_time_t");
}

inline bool pstsdk::test_bit(const byte* pbytes, ulong bit)
{
    return (*(pbytes + (bit >> 3)) & (0x80 >> (bit & 7))) != 0;
}

#if defined(_WIN32) || defined(__MINGW32__)

// We know that std::wstring is always UCS-2LE on Windows.

inline std::wstring pstsdk::bytes_to_wstring(const std::vector<byte> &bytes)
{
    if(bytes.size() == 0)
        return std::wstring();

    return std::wstring(reinterpret_cast<const wchar_t *>(&bytes[0]), bytes.size()/sizeof(wchar_t));
}

inline std::vector<pstsdk::byte> pstsdk::wstring_to_bytes(const std::wstring &wstr)
{
    if(wstr.size() == 0)
        return std::vector<byte>();

    const byte *begin = reinterpret_cast<const byte *>(&wstr[0]);
    return std::vector<byte>(begin, begin + wstr.size()*sizeof(wchar_t));
}

#else // !(defined(_WIN32) || defined(__MINGW32__))

// We're going to have to do this the hard way, since we don't know how
// big wchar_t really is, or what encoding it uses.
#include <iconv.h>

inline std::wstring pstsdk::bytes_to_wstring(const std::vector<byte> &bytes)
{
    if(bytes.size() == 0)
        return std::wstring();

    // Up to one wchar_t for every 2 bytes, if there are no surrogate pairs.
    if(bytes.size() % 2 != 0)
        throw std::runtime_error("Cannot interpret odd number of bytes as UTF-16LE");
    std::wstring out(bytes.size() / 2, L'\0');

    iconv_t cd(::iconv_open("WCHAR_T", "UTF-16LE"));
    if(cd == (iconv_t)(-1)) {
        perror("bytes_to_wstring");
        throw std::runtime_error("Unable to convert from UTF-16LE to wstring");
    }

    const char *inbuf = reinterpret_cast<const char *>(&bytes[0]);
    size_t inbytesleft = bytes.size();
    char *outbuf = reinterpret_cast<char *>(&out[0]);
    size_t outbytesleft = out.size() * sizeof(wchar_t);
    size_t result = ::iconv(cd, const_cast<char **>(&inbuf), &inbytesleft, &outbuf, &outbytesleft);
    ::iconv_close(cd);
    if(result == (size_t)(-1) || inbytesleft > 0 || outbytesleft % sizeof(wchar_t) != 0)
        throw std::runtime_error("Failed to convert from UTF-16LE to wstring");

    out.resize(out.size() - (outbytesleft / sizeof(wchar_t)));
    return out;
}

inline std::vector<pstsdk::byte> pstsdk::wstring_to_bytes(const std::wstring &wstr)
{
    if(wstr.size() == 0)
        return std::vector<byte>();

    // Up to 4 bytes per character if all codepoints are surrogate pairs.
    std::vector<byte> out(wstr.size() * 4);

    iconv_t cd(::iconv_open("UTF-16LE", "WCHAR_T"));
    if(cd == (iconv_t)(-1)) {
        perror("wstring_to_bytes");
        throw std::runtime_error("Unable to convert from wstring to UTF-16LE");
    }

    const char *inbuf = reinterpret_cast<const char *>(&wstr[0]);
    size_t inbytesleft = wstr.size() * sizeof(wchar_t);
    char *outbuf = reinterpret_cast<char *>(&out[0]);
    size_t outbytesleft = out.size();
    size_t result = ::iconv(cd, const_cast<char **>(&inbuf), &inbytesleft, &outbuf, &outbytesleft);
    ::iconv_close(cd);
    if(result == (size_t)(-1) || inbytesleft > 0)
        throw std::runtime_error("Failed to convert from wstring to UTF-16LE");

    out.resize(out.size() - outbytesleft);
    return out;
}

#endif // !(defined(_WIN32) || defined(__MINGW32__))

#endif
