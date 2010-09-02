//! \file
//! \brief The exceptions used by pstsdk
//! \author Terry Mahaffey
//! \ingroup util

//! \defgroup exception Exceptions
//! \ingroup util

#ifndef PSTSDK_UTIL_ERRORS_H
#define PSTSDK_UTIL_ERRORS_H

#include <stdexcept>
#include "pstsdk/util/primitives.h"

namespace pstsdk
{

//! \brief A block or node can not satisfy a resize request
//! \ingroup exception
class can_not_resize : public std::runtime_error
{
public:
    explicit can_not_resize(const std::string& error)
        : runtime_error(error) { }
};

//! \brief This function or method has not been implemented.
//! \ingroup exception
class not_implemented : public std::logic_error
{
public:
    explicit not_implemented(const std::string& error)
        : logic_error(error) { }
};

//! \brief An error occured writing to the file.
//! \ingroup exception
class write_error : public std::runtime_error
{
public:
    explicit write_error(const std::string& error)
        : runtime_error(error) { }
};

//! \brief The database is corrupt.
//! \ingroup exception
class database_corrupt : public std::runtime_error 
{
public:
    explicit database_corrupt(const std::string& error)
        : runtime_error(error) { }
};

//! \brief The database was not in the expected format.
//! \ingroup exception
class invalid_format : public database_corrupt
{
public:
    invalid_format()
        : database_corrupt("Unexpected Database Format") { }
};

//! \brief An unexpected page or page type was encountered.
//! \ingroup exception
class unexpected_page : public database_corrupt
{
public:
    explicit unexpected_page(const std::string& error)
        : database_corrupt(error) { }
};

//! \brief An unexpected block or block type was encountered.
//! \ingroup exception
class unexpected_block : public database_corrupt
{
public:
    explicit unexpected_block(const std::string& error)
        : database_corrupt(error) { }
};

//! \brief A CRC of an item failed.
//! \ingroup exception
class crc_fail : public database_corrupt
{
public:
    crc_fail(const std::string& error, ulonglong location, block_id id, ulong actual, ulong expected)
        : database_corrupt(error), m_location(location), m_id(id), m_actual(actual), m_expected(expected) { }
private:
    ulonglong m_location;   //!< The location where this item was located
    block_id m_id;          //!< The id of the item
    ulong m_actual;         //!< The actual or calculated CRC
    ulong m_expected;       //!< The expected CRC value
};

//! \brief An unexpected signature was encountered.
//! \ingroup exception
class sig_mismatch : public database_corrupt
{
public:
    sig_mismatch(const std::string& error, ulonglong location, block_id id, ulong actual, ulong expected)
        : database_corrupt(error), m_location(location), m_id(id), m_actual(actual), m_expected(expected) { }
private:
    ulonglong m_location;   //!< The location where this signature was expected
    block_id m_id;          //!< The id of the item which should have had this signature
    ulong m_actual;         //!< The actual signature
    ulong m_expected;       //!< The expected signature
};

//! \brief The requested key was not found.
//! \param K The key type
//! \ingroup exception
template<typename K>
class key_not_found : public std::exception
{
public:
    explicit key_not_found(const K& k)
        : m_k(k) { }
    virtual ~key_not_found() throw() { }

    const char* what() const throw()
        { return "key not found"; }

    //! \brief Returns the key which was not found
    //! \returns The missing key
    const K& which() const
        { return m_k; }
private:
    key_not_found<K>& operator=(const key_not_found<K>&);
    const K m_k;
};


} // end namespace

#endif
