//! \file
//! \brief Named Property Lookup Map implementation
//! \author Terry Mahaffey
//! \ingroup ltp

#ifndef PSTSDK_LTP_NAMEID_H
#define PSTSDK_LTP_NAMEID_H

#include <string>
#include <algorithm>

#include "pstsdk/util/primitives.h"

#include "pstsdk/ndb/database_iface.h"

#include "pstsdk/ltp/propbag.h"

namespace pstsdk
{

//! \defgroup ltp_namedproprelated Named Properties
//! \ingroup ltp

//! \brief A named property
//!
//! The equivalent of a MAPI named property. This is one half of the named
//! property/prop id mapping. This is used as both a key and value type, 
//! depending on which way in the named property mapping you're going.
//! \ingroup ltp_namedproprelated
class named_prop
{
public:
    //! \brief Construct a named prop from a guid and an id
    //! \param[in] g The GUID which this named prop is a member
    //! \param[in] id The numerical id for this property
    named_prop(const guid& g, long id) 
        : m_guid(g), m_is_string(false), m_id(id) { }
    //! \brief Construct a named prop from a guid and a string
    //! \param[in] g The GUID which this named prop is a member
    //! \param[in] name The string name for this property
    named_prop(const guid& g, const std::wstring& name)
        : m_guid(g), m_is_string(true), m_name(name) { }

    //! \brief Get the namespace GUID of which this named prop is a member
    //! \returns The GUID for the namespace this named property is in
    const guid& get_guid() const { return m_guid; }
    //! \brief Return true if this is a string named prop
    //! \returns true if this is a string named prop
    bool is_string() const { return m_is_string; }
    //! \brief Get the numerical id of this property
    //! \pre is_string() == false
    //! \returns The numerical id of this named prop
    long get_id() const { return m_id; }
    //! \brief Get the name of this property
    //! \pre is_string() == true
    //! \returns The name of this property
    const std::wstring& get_name() const { return m_name; }

private:
    guid m_guid;            //!< The namespace GUID this property is a member of
    bool m_is_string;       //!< True if this is a string named prop
    long m_id;              //!< The id of this property
    std::wstring m_name;    //!< The name of this property
};

//! \brief A named property map abstraction
//!
//! This class abstractions away the logic of doing named property lookups
//! in a message store.
//! 
//! To use this class, one just constructs it with a store pointer
//! and calls the various lookup overloads as needed.
//! \sa [MS-PST] 2.4.7
//! \ingroup ltp_namedproprelated
class name_id_map : private boost::noncopyable
{
public:
    //! \brief Construct a name_id_map for the given store
    //!
    //! This will open the name_id_map node for the store
    //! \param db The store to get the named property mapping for
    name_id_map(const shared_db_ptr& db) 
        : m_bag(db->lookup_node(nid_name_id_map)), m_buckets(m_bag.read_prop<slong>(0x1)), m_entry_stream(m_bag.open_prop_stream(0x3)), m_guid_stream(m_bag.open_prop_stream(0x2)), m_string_stream(m_bag.open_prop_stream(0x4)) { }

    //! \brief Query if a given named prop exists
    //! \param[in] g The namespace guid for the named prop
    //! \param[in] name The name of the named prop
    //! \returns true If the named property has a prop_id mapped
    bool name_exists(const guid& g, const std::wstring& name) const
        { return named_prop_exists(named_prop(g, name)); }
    //! \brief Query if a given named prop exists
    //! \param[in] g The namespace guid for the named prop
    //! \param[in] id The name of the named prop
    //! \returns true If the named property has a prop_id mapped
    bool id_exists(const guid& g, long id) const
        { return named_prop_exists(named_prop(g, id)); }
    //! \brief Query if a given named prop exists
    //! \param[in] p A constructed named_prop object
    //! \returns true If the named property has a prop_id mapped
    bool named_prop_exists(const named_prop& p) const;
    //! \brief Query if a given prop_id has a named_prop mapped to it
    //! \param[in] id The prop_id to check
    //! \returns true If the prop_id has a named_prop mapped
    bool prop_id_exists(prop_id id) const;
    //! \brief Get the total count of named property mappings in this store
    //! \returns The count of named property mappings in this store
    size_t get_prop_count() const;

    //! \brief Get all of the prop_ids which have a named_prop mapping in this store
    //! \returns a vector of prop_ids
    std::vector<prop_id> get_prop_list() const;

    //! \brief Get the associated prop_id of the named property
    //! \throws key_not_found<named_prop> If the named prop doesn't have a prop_id mapping
    //! \param[in] g The namespace guid for the named prop
    //! \param[in] name The name of the named prop
    //! \returns The mapped prop_id of this named property
    prop_id lookup(const guid& g, const std::wstring& name) const
        { return lookup(named_prop(g, name)); }
    //! \brief Get the associated prop_id of the named property
    //! \throws key_not_found<named_prop> If the named prop doesn't have a prop_id mapping
    //! \param[in] g The namespace guid for the named prop
    //! \param[in] id The name of the named prop
    //! \returns The mapped prop_id of this named property
    prop_id lookup(const guid& g, long id) const
        { return lookup(named_prop(g, id)); }
    //! \brief Get the associated prop_id of the named property
    //! \throws key_not_found<named_prop> If the named prop doesn't have a prop_id mapping
    //! \param[in] p A constructed named_prop object
    //! \returns The mapped prop_id of this named property
    prop_id lookup(const named_prop& p) const;

    //! \brief Get the associated named_prop of this prop_id
    //! \throws key_not_found<prop_id> If the prop_id doesn't have a named_prop mapping
    //! \param[in] id The prop id to lookup
    //! \returns The associated named_prop object
    named_prop lookup(prop_id id) const;

private:
    // helper functions
    named_prop construct(const disk::nameid& entry) const;
    named_prop construct(ulong index) const;
    //! \brief Given a guid index into the GUID stream, return the namespace GUID
    //! \sa [MS-PST] 2.4.7.1/wGuid
    //! \param[in] guid_index The index into the guid stream
    //! \returns The namespace GUID
    guid read_guid(ushort guid_index) const;
    ushort get_guid_index(const guid& g) const;
    std::wstring read_wstring(ulong string_offset) const;
    ulong compute_hash_base(const named_prop& n) const; 
    ulong compute_hash_value(ushort guid_index, const named_prop& n) const
        { return (n.is_string() ? ((guid_index << 1) | 1) : (guid_index << 1)) ^ compute_hash_base(n); }
    prop_id get_bucket_prop(ulong hash_value) const
        { return static_cast<prop_id>((hash_value % m_buckets) + 0x1000); }

    property_bag m_bag;                     //!< The property bag backing the named prop map
    ulong m_buckets;                        //!< The number of buckets in the property bag, [NS-PST] 2.4.7.5
    mutable prop_stream m_entry_stream;     //!< The entry stream, [MS-PST] 2.4.7.3
    mutable prop_stream m_guid_stream;      //!< The guid stream, [MS-PST] 2.4.7.2
    mutable prop_stream m_string_stream;    //!< The string stream, [MS-PST] 2.4.7.4
};

inline pstsdk::named_prop pstsdk::name_id_map::construct(const disk::nameid& entry) const
{
    if(nameid_is_string(entry))
        return named_prop(read_guid(disk::nameid_get_guid_index(entry)), read_wstring(entry.string_offset));
    else
        return named_prop(read_guid(disk::nameid_get_guid_index(entry)), entry.id);
}

inline pstsdk::named_prop pstsdk::name_id_map::construct(ulong index) const
{
    disk::nameid entry;
    m_entry_stream.seekg(index * sizeof(disk::nameid), std::ios_base::beg);
    m_entry_stream.read((char*)&entry, sizeof(entry));
    return construct(entry);
}

inline pstsdk::guid pstsdk::name_id_map::read_guid(ushort guid_index) const
{
    if(guid_index == 0)
        return ps_none;
    if(guid_index == 1)
        return ps_mapi;
    if(guid_index == 2)
        return ps_public_strings;

    guid g;
    m_guid_stream.seekg((guid_index-3) * sizeof(guid), std::ios_base::beg);
    m_guid_stream.read((char*)&g, sizeof(g));
    return g;
}

inline pstsdk::ushort pstsdk::name_id_map::get_guid_index(const guid& g) const
{
    if(memcmp(&g, &ps_none, sizeof(g)) == 0)
        return 0;
    if(memcmp(&g, &ps_mapi, sizeof(g)) == 0)
        return 1;
    if(memcmp(&g, &ps_public_strings, sizeof(g)) == 0)
        return 2;

    guid g_disk;
    ushort num = 0;
    m_guid_stream.seekg(0, std::ios_base::beg);
    while(m_guid_stream.read((char*)&g_disk, sizeof(g_disk)))
    {
        if(memcmp(&g, &g_disk, sizeof(g)) == 0)
            return num + 3;
        ++num;
    }

    // didn't find it
    m_guid_stream.clear();
    throw key_not_found<guid>(g);
}

inline std::wstring pstsdk::name_id_map::read_wstring(ulong string_offset) const
{
    m_string_stream.seekg(string_offset, std::ios_base::beg);

    ulong size;
    m_string_stream.read((char*)&size, sizeof(size));

    std::vector<byte> buffer(size);
    m_string_stream.read(reinterpret_cast<char *>(&buffer[0]), size);

    return bytes_to_wstring(buffer);
}

inline pstsdk::ulong pstsdk::name_id_map::compute_hash_base(const named_prop& n) const {
    if(n.is_string())
    {
        std::vector<byte> bytes(wstring_to_bytes(n.get_name()));
        return disk::compute_crc(&bytes[0], bytes.size());
    }
    else
    {
        return n.get_id();
    }
}

inline bool pstsdk::name_id_map::named_prop_exists(const named_prop& p) const
{
    try 
    {
        lookup(p);
        return true;
    }
    catch(std::exception&)
    {
        return false;
    }
}

inline bool pstsdk::name_id_map::prop_id_exists(prop_id id) const
{
    if(id >= 0x8000)
        return static_cast<size_t>((id - 0x8000)) < get_prop_count();

    // id < 0x8000 is a ps_mapi prop
    return true;
}

inline std::vector<prop_id> pstsdk::name_id_map::get_prop_list() const
{
    disk::nameid entry;
    std::vector<prop_id> props;

    m_entry_stream.seekg(0, std::ios_base::beg);
    while(m_entry_stream.read((char*)&entry, sizeof(entry)) != 0)
        props.push_back(nameid_get_prop_index(entry) + 0x8000);

    m_entry_stream.clear();

    return props;
}

inline size_t pstsdk::name_id_map::get_prop_count() const
{
    m_entry_stream.seekg(0, std::ios_base::end);

    return static_cast<size_t>(m_entry_stream.tellg()) / sizeof(disk::nameid);
}

inline pstsdk::prop_id pstsdk::name_id_map::lookup(const named_prop& p) const
{
    ushort guid_index;
    
    try
    {
        guid_index = get_guid_index(p.get_guid());
    }
    catch(key_not_found<guid>&)
    {
        throw key_not_found<named_prop>(p);
    }

    // special handling of ps_mapi
    if(guid_index == 1)
    {
        if(p.is_string()) throw key_not_found<named_prop>(p);
        if(p.get_id() >= 0x8000) throw key_not_found<named_prop>(p);
        return static_cast<prop_id>(p.get_id());
    }

    ulong hash_value = compute_hash_value(guid_index, p);
    ulong hash_base = compute_hash_base(p);

    if(!m_bag.prop_exists(get_bucket_prop(hash_value)))
        throw key_not_found<named_prop>(p);

    prop_stream bucket(const_cast<name_id_map*>(this)->m_bag.open_prop_stream(get_bucket_prop(hash_value)));
    disk::nameid_hash_entry entry;
    while(bucket.read((char*)&entry, sizeof(entry)) != 0)
    {
        if( (entry.hash_base == hash_base) &&
            (disk::nameid_is_string(entry) == p.is_string()) &&
            (disk::nameid_get_guid_index(entry) == guid_index)
        )
        {
            // just double check the string..
            if(p.is_string())
            {
                if(construct(disk::nameid_get_prop_index(entry)).get_name() != p.get_name())
                    continue;
            }

            // found it!
            return disk::nameid_get_prop_index(entry) + 0x8000;
        }
    }

    throw key_not_found<named_prop>(p);
}

inline pstsdk::named_prop pstsdk::name_id_map::lookup(prop_id id) const
{
    if(id < 0x8000)
        return named_prop(ps_mapi, id);

    ulong index = id - 0x8000;

    if(index > get_prop_count())
        throw key_not_found<prop_id>(id);

    return construct(index);
}

} // end namespace pstsdk

#endif
