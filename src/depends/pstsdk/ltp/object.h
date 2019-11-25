//! \file
//! \brief Property access base class
//! \author Terry Mahaffey
//! 
//! This file contains the base class used to access "properties" on "objects".
//! It defines several virtual methods to access raw properties, and 
//! child classes (specific object types) implement them. 
//!
//! In addition to defining a generic read_prop<T>(id) interface, it also
//! defines a stream interface to access large properties.
//! \ingroup ltp

#ifndef PSTSDK_LTP_OBJECT_H
#define PSTSDK_LTP_OBJECT_H

#include <functional>
#ifdef __GNUC__
#include <tr1/type_traits>
#include <tr1/functional>
#else
#include <type_traits>
#endif
#include <algorithm>
#include <boost/iostreams/concepts.hpp>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif
#include <boost/iostreams/stream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "pstsdk/util/primitives.h"
#include "pstsdk/util/errors.h"

#include "pstsdk/ltp/heap.h"

#include "pstsdk/ndb/node.h"

namespace pstsdk
{

//! \defgroup ltp_objectrelated Property Objects
//! \ingroup ltp

//! \brief Defines a stream device which can wrap one of the two prop sources
//!
//! An hnid_steam_device wraps either a \ref hid_stream_device if the property
//! being streamed is located in a heap, or a \ref node_stream_device if the
//! property being streamed is contained in it's own subnode.
//!
//! It is a common feature in both types of property objects to allocate
//! properties in the heap first, and then "roll over" to a subnode when/if
//! the size of the property becomes too large to fit in a heap allocation.
//! \sa [MS-PST] 2.3.3.2
//! \ingroup ltp_objectrelated
class hnid_stream_device : public boost::iostreams::device<boost::iostreams::input_seekable>
{
public:
    //! \copydoc node_stream_device::read()
    std::streamsize read(char* pbuffer, std::streamsize n)
        { if(m_is_hid) return m_hid_device.read(pbuffer, n); else return m_node_device.read(pbuffer, n); }
    //! \copydoc node_stream_device::seek()
    std::streampos seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way)
        { if(m_is_hid) return m_hid_device.seek(off, way); else return m_node_device.seek(off, way); }

    //! \brief Wrap a hid_stream_device
    hnid_stream_device(const hid_stream_device& hid_device) : m_hid_device(hid_device), m_is_hid(true) { }
    //! \brief Wrap a node_stream_device
    hnid_stream_device(const node_stream_device& node_device) : m_node_device(node_device), m_is_hid(false) { }

private:
    hid_stream_device m_hid_device;
    node_stream_device m_node_device;
    bool m_is_hid;

    std::streamsize m_pos;
};

//! \brief The actual property stream, defined using the boost iostream library
//! and the \ref hnid_stream_device.
//! \ingroup ltp_objectrelated
typedef boost::iostreams::stream<hnid_stream_device> prop_stream;

//! \brief Property object base class
//!
//! This class provides a simple interface (using templates) to access
//! properties on an object. It defines a handful of virtual functions
//! child classes implement to read the raw data.
//!
//! This class is contains all the conversion logic between the different
//! types, implemented as template specializations, and other property
//! logic such as multivalued property parsing.
//! \ingroup ltp_objectrelated
class const_property_object
{
public:
    virtual ~const_property_object() { }
    //! \brief Get a list of all properties on this object
    //! \returns A vector of all properties on this object
    virtual std::vector<prop_id> get_prop_list() const = 0;
    //! \brief Get the property type of a given prop_id
    //! \param[in] id The prop_id
    //! \throws key_not_found<prop_id> If the specified property is not present
    //! \returns The type of the prop_id
    virtual prop_type get_prop_type(prop_id id) const = 0;
    //! \brief Indicates the existance of a given property on this object
    //! \param[in] id The prop_id
    //! \returns true if the property exists
    virtual bool prop_exists(prop_id id) const = 0;
    //! \brief Returns the total size of a variable length property
    //! \note This operation is only valid for variable length properties
    //! \param[in] id The prop_id
    //! \returns The vector.size() if read_prop was called for this prop
    virtual size_t size(prop_id id) const = 0;

    //! \brief Read a property as a given type
    //!
    //! It is the callers responsibility to ensure the prop_id is of or 
    //! convertable to the requested type.
    //! \tparam T The type to interpret he property as
    //! \param[in] id The prop_id
    //! \throws key_not_found<prop_id> If the specified property is not present
    //! \returns The property value
    template<typename T>
    T read_prop(prop_id id) const;

    //! \brief Read a property as an array of the given type
    //!
    //! It is the callers responsibility to ensure the prop_id is of or 
    //! convertable to the requested type.
    //! \tparam T The type to interpret he property as
    //! \param[in] id The prop_id
    //! \throws key_not_found<prop_id> If the specified property is not present
    //! \returns A vector of the property values
    //! \sa [MS-PST] 2.3.3.4
    template<typename T>
    std::vector<T> read_prop_array(prop_id id) const;

    //! \brief Creates a stream device over a property on this object
    //!
    //! The returned stream device can be used to construct a proper stream:
    //! \code
    //! const_property_object* po = ...;
    //! prop_stream nstream(po->open_prop_stream(0x3001));
    //! \endcode
    //! Which can then be used as any iostream would be.
    //! \note This is operation is only valid for variable length properties
    //! \param[in] id The prop_id
    //! \throws key_not_found<prop_id> If the specified property is not present
    //! \returns A stream device for the requested property
    virtual hnid_stream_device open_prop_stream(prop_id id) = 0;

// GCC has time_t defined as a typedef of a long, so calling
// read_prop<slong> activates the time_t specialization. I'm
// turning them into first class member functions in GCC for now until
// I figure out a portable way to deal with time.
#ifdef __GNUC__
    time_t read_time_t_prop(prop_id id) const;
    std::vector<time_t> read_time_t_array(prop_id id) const;
#endif

protected:
    //! \brief Implemented by child classes to fetch a 1 byte sized property
    virtual byte get_value_1(prop_id id) const = 0;
    //! \brief Implemented by child classes to fetch a 2 byte sized property
    virtual ushort get_value_2(prop_id id) const = 0;
    //! \brief Implemented by child classes to fetch a 4 byte sized property
    virtual ulong get_value_4(prop_id id) const = 0;
    //! \brief Implemented by child classes to fetch a 8 byte sized property
    virtual ulonglong get_value_8(prop_id id) const = 0;
    //! \brief Implemented by child classes to fetch a variable sized property
    virtual std::vector<byte> get_value_variable(prop_id id) const = 0;
};

} // end pstsdk namespace

template<typename T>
inline T pstsdk::const_property_object::read_prop(prop_id id) const
{
#ifdef _MSC_VER
#pragma warning(suppress:4127)
#endif
    if(!std::tr1::is_pod<T>::value)
        throw std::invalid_argument("T must be a POD or one of the specialized classes");

    if(sizeof(T) == sizeof(ulonglong))
    {
        ulonglong t = get_value_8(id);
        return *(reinterpret_cast<T*>(&t));
    }
    else if(sizeof(T) == sizeof(ulong))
    {
        ulong t = get_value_4(id);
        return *(reinterpret_cast<T*>(&t));
    }
    else if(sizeof(T) == sizeof(ushort))
    {
        ushort t = get_value_2(id);
        return *(reinterpret_cast<T*>(&t));
    }
    else if(sizeof(T) == sizeof(byte))
    {
        byte t = get_value_1(id);
        return *(reinterpret_cast<T*>(&t));
    }
    else
    {
        std::vector<byte> buffer = get_value_variable(id);
        return *(reinterpret_cast<T*>(&buffer[0]));
    }
}

template<typename T>
inline std::vector<T> pstsdk::const_property_object::read_prop_array(prop_id id) const
{
#ifdef _MSC_VER
#pragma warning(suppress:4127)
#endif
    if(!std::tr1::is_pod<T>::value)
        throw std::invalid_argument("T must be a POD or one of the specialized classes");

    std::vector<byte> buffer = get_value_variable(id); 
    return std::vector<T>(reinterpret_cast<T*>(&buffer[0]), reinterpret_cast<T*>(&buffer[0] + buffer.size()));
}

namespace pstsdk
{

template<>
inline bool const_property_object::read_prop<bool>(prop_id id) const
{
    return get_value_4(id) != 0;
}

template<>
inline std::vector<bool> pstsdk::const_property_object::read_prop_array<bool>(prop_id id) const
{
    using namespace std::tr1::placeholders;

    std::vector<ulong> values = read_prop_array<ulong>(id);
    std::vector<bool> results(values.size());
    std::transform(values.begin(), values.end(), results.begin(), std::tr1::bind(std::not_equal_to<ulong>(), 0, _1));
    return results;
}

// See the note in the class definition - convert the time_t read_prop
// specialization into a member function in GCC
#ifdef __GNUC__
inline time_t const_property_object::read_time_t_prop(prop_id id) const
#else
template<>
inline time_t const_property_object::read_prop<time_t>(prop_id id) const
#endif
{
    if(get_prop_type(id) == prop_type_apptime)
    {
        double time_value = read_prop<double>(id);
        return vt_date_to_time_t(time_value);
    }
    else
    {
        ulonglong time_value = read_prop<ulonglong>(id);
        return filetime_to_time_t(time_value);
    }
}

#ifdef __GNUC__
inline std::vector<time_t> const_property_object::read_time_t_array(prop_id id) const
#else
template<>
inline std::vector<time_t> const_property_object::read_prop_array<time_t>(prop_id id) const
#endif
{
    if(get_prop_type(id) == prop_type_mv_apptime)
    {
        std::vector<double> time_values = read_prop_array<double>(id);
        std::vector<time_t> result(time_values.size());
        std::transform(time_values.begin(), time_values.end(), result.begin(), vt_date_to_time_t);
        return result;
    }
    else
    {
        std::vector<ulonglong> time_values = read_prop_array<ulonglong>(id);
        std::vector<time_t> result(time_values.size());
        std::transform(time_values.begin(), time_values.end(), result.begin(), filetime_to_time_t);
        return result;
    }

}

template<>
inline std::vector<byte> const_property_object::read_prop<std::vector<byte> >(prop_id id) const
{
    return get_value_variable(id); 
}

template<>
inline std::vector<std::vector<byte> > const_property_object::read_prop_array<std::vector<byte> >(prop_id id) const
{
    std::vector<byte> buffer = get_value_variable(id);
#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(buffer.size() < sizeof(ulong))
        throw std::length_error("mv prop too short");
#endif
    disk::mv_toc* ptoc = reinterpret_cast<disk::mv_toc*>(&buffer[0]);
    std::vector<std::vector<byte> > results;

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(buffer.size() < (sizeof(ulong) + ptoc->count * sizeof(ulong)))
        throw std::length_error("mv prop too short");
#endif

    for(ulong i = 0; i < ptoc->count; ++i)
    {
        ulong start = ptoc->offsets[i];
        ulong end = (i == (ptoc->count - 1)) ? buffer.size() : ptoc->offsets[i+1];
#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
        if(end < start)
            throw std::length_error("inconsistent mv prop toc");
#endif
        results.push_back(std::vector<byte>(&buffer[start], &buffer[start] + (end-start)));
    }

    return results;
}

template<>
inline std::wstring const_property_object::read_prop<std::wstring>(prop_id id) const
{
    std::vector<byte> buffer = get_value_variable(id); 

    if(get_prop_type(id) == prop_type_string)
    {
        std::string s(buffer.begin(), buffer.end());
        return std::wstring(s.begin(), s.end());
    }
    else
    {
        return bytes_to_wstring(buffer);
    }
}

template<>
inline std::vector<std::wstring> const_property_object::read_prop_array<std::wstring>(prop_id id) const
{
    std::vector<std::vector<byte> > buffer = read_prop_array<std::vector<byte> >(id);
    std::vector<std::wstring> results;

    for(size_t i = 0; i < buffer.size(); ++i)
    {
        if(get_prop_type(id) == prop_type_mv_string)
        {
            std::string s(buffer[i].begin(), buffer[i].end());
            results.push_back(std::wstring(s.begin(), s.end()));
        }
        else
        {
            results.push_back(bytes_to_wstring(buffer[i]));
        }
    }

    return results;
}

template<>
inline std::string const_property_object::read_prop<std::string>(prop_id id) const
{
    std::vector<byte> buffer = get_value_variable(id); 

    if(get_prop_type(id) == prop_type_string)
    {
        return std::string(buffer.begin(), buffer.end());
    }
    else
    {
        if(buffer.size())
        {
            std::wstring s(bytes_to_wstring(buffer));
            return std::string(s.begin(), s.end());
        }
        return std::string();
    }
}

template<>
inline std::vector<std::string> const_property_object::read_prop_array<std::string>(prop_id id) const
{
    std::vector<std::vector<byte> > buffer = read_prop_array<std::vector<byte> >(id);
    std::vector<std::string> results;

    for(size_t i = 0; i < buffer.size(); ++i)
    {
        if(get_prop_type(id) == prop_type_mv_string)
        {
            results.push_back(std::string(buffer[i].begin(), buffer[i].end()));
        }
        else
        {
            if(buffer[i].size())
            {
                std::wstring s(bytes_to_wstring(buffer[i]));
                results.push_back(std::string(s.begin(), s.end()));
            }
            results.push_back(std::string());
        }
    }

    return results;
}

} // end pstsdk namespace

#endif
