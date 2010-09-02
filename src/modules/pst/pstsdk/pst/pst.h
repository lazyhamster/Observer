//! \file
//! \brief PST implementation
//!
//! This file contains the implementation of the "pst" object, the object that
//! most users of the library will be most familiar with. It's the entry point
//! to the pst file, allowing you to search for, enumerate over, and directly 
//! open the sub objects which contain the actual data.
//!
//! Named property resolution was also exposed on this object.
//! \author Terry Mahaffey
//! \ingroup pst

#ifndef PSTSDK_PST_PST_H
#define PSTSDK_PST_PST_H

#include <boost/noncopyable.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "pstsdk/ndb/database.h"
#include "pstsdk/ndb/database_iface.h"
#include "pstsdk/ndb/node.h"

#include "pstsdk/ltp/propbag.h"
#include "pstsdk/ltp/nameid.h"

#include "pstsdk/pst/folder.h"
#include "pstsdk/pst/message.h"

namespace pstsdk
{

//! \defgroup pst_pstrelated PST
//! \ingroup pst

//! \brief A PST file
//!
//! pst represents a pst file on disk. Both OST and PST files are supported,
//! both ANSI and Unicode. Once a client creates a PST file, this object
//! supports:
//! - iterating over all messages in the store
//! - iterating over all folders in the store
//! - opening the root folder
//! - opening a specific folder by name
//! - performing named property lookups
//! \ingroup pst_pstrelated
class pst : private boost::noncopyable
{
    typedef boost::filter_iterator<is_nid_type<nid_type_folder>, const_nodeinfo_iterator> folder_filter_iterator;
    typedef boost::filter_iterator<is_nid_type<nid_type_message>, const_nodeinfo_iterator> message_filter_iterator;

public:
    //! \brief Message iterator type; a transform iterator over a filter iterator over a nodeinfo iterator
    typedef boost::transform_iterator<message_transform_info, message_filter_iterator> message_iterator;
    //! \brief Folder iterator type; a transform iterator over a filter iterator over a nodeinfo iterator
    typedef boost::transform_iterator<folder_transform_info, folder_filter_iterator> folder_iterator;

    //! \brief Construct a pst object from the specified file
    //! \param[in] filename The pst file to open on disk
    pst(const std::wstring& filename) 
        : m_db(open_database(filename)) { }

#ifndef BOOST_NO_RVALUE_REFERENCES
    //! \brief Move constructor
    //! \param[in] other The other pst file
    pst(pst&& other)
        : m_db(std::move(other.m_db)), m_bag(std::move(other.m_bag)), m_map(std::move(other.m_map)) { }
#endif

    // subobject discovery/enumeration
    //! \brief Get an iterator to the first folder in the PST file
    //! \returns an iterator positioned on the first folder in this PST file
    folder_iterator folder_begin() const
        { return boost::make_transform_iterator(boost::make_filter_iterator<is_nid_type<nid_type_folder> >(m_db->read_nbt_root()->begin(), m_db->read_nbt_root()->end()), folder_transform_info(m_db) ); }
    //! \brief Get the end folder iterator
    //! \returns an iterator at the end position
    folder_iterator folder_end() const
        { return boost::make_transform_iterator(boost::make_filter_iterator<is_nid_type<nid_type_folder> >(m_db->read_nbt_root()->end(), m_db->read_nbt_root()->end()), folder_transform_info(m_db) ); }

    //! \brief Get an iterator to the first message in the PST file
    //! \returns an iterator positioned on the first message in this PST file
    message_iterator message_begin() const
        { return boost::make_transform_iterator(boost::make_filter_iterator<is_nid_type<nid_type_message> >(m_db->read_nbt_root()->begin(), m_db->read_nbt_root()->end()), message_transform_info(m_db) ); }
    //! \brief Get the end message iterator
    //! \returns an iterator at the end position
    message_iterator message_end() const
        { return boost::make_transform_iterator(boost::make_filter_iterator<is_nid_type<nid_type_message> >(m_db->read_nbt_root()->end(), m_db->read_nbt_root()->end()), message_transform_info(m_db) ); }

    //! \brief Opens the root folder of this file
    //! \note This is specific to PST files, as an OST file has a different root folder
    //! \returns The root of the folder hierarchy in this file
    folder open_root_folder() const
        { return folder(m_db, m_db->lookup_node(nid_root_folder)); }
    //! \brief Open a specific folder in this file
    //! \param[in] name The name of the folder to open
    //! \throws key_not_found<std::wstring> If a folder of the specified name was not found in this file
    //! \returns The first folder by that name found in the file
    folder open_folder(const std::wstring& name) const;

    //! \brief Open a specific message in this file
    //! \param[in] name The node_id of the message to open
    //! \throws key_not_found<node_id> If a folder of the specified id was not found in this file
    //! \returns The folder with that id found in the file
    folder open_folder(node_id id) const
        { return folder(m_db, m_db->lookup_node(id)); }

    //! \brief Open a specific message in this file
    //! \param[in] name The node_id of the message to open
    //! \throws key_not_found<node_id> If a search_folder of the specified id was not found in this file
    //! \returns The search_folder with that id found in the file
    search_folder open_search_folder(node_id id) const
        { return search_folder(m_db, m_db->lookup_node(id)); }

    //! \brief Open a specific message in this file
    //! \param[in] name The node_id of the message to open
    //! \throws key_not_found<node_id> If a message of the specified id was not found in this file
    //! \returns The message with that id found in the file
    message open_message(node_id id) const
        { return message(m_db->lookup_node(id)); }

    // property access
    //! \brief Get the display name of the PST
    //! \returns The display name
    std::wstring get_name() const
        { return get_property_bag().read_prop<std::wstring>(0x3001); }
    //! \brief Lookup a prop_id of a named prop
    //! \param[in] g The namespace guid of the named prop to lookup
    //! \param[in] name The name of the property to lookup
    //! \returns The prop_id of the property looked up
    prop_id lookup_prop_id(const guid& g, const std::wstring& name) const
        { return get_name_id_map().lookup(g, name); }
    //! \brief Lookup a prop_id of a named prop
    //! \param[in] g The namespace guid of the named prop to lookup
    //! \param[in] id The id of the property to lookup
    //! \returns The prop_id of the property looked up
    prop_id lookup_prop_id(const guid& g, long id) const
        { return get_name_id_map().lookup(g, id); }
    //! \brief Lookup a prop_id of a named prop
    //! \param[in] n The named prop to lookup
    //! \returns The prop_id of the property looked up
    prop_id lookup_prop_id(const named_prop& n)
        { return get_name_id_map().lookup(n); }
    //! \brief Lookup a named prop of a prop_id
    //! \param[in] id The prop_id to lookup
    //! \returns The mapped named property
    named_prop lookup_name_prop(prop_id id) const
        { return get_name_id_map().lookup(id); }

    // lower layer access
    //! \brief Get the property bag of the store object
    //! \returns The property bag
    property_bag& get_property_bag();
    //! \brief Get the named prop map for this store
    //! \returns The named property map
    name_id_map& get_name_id_map();
    //! \brief Get the property bag of the store object
    //! \returns The property bag
    const property_bag& get_property_bag() const;
    //! \brief Get the named prop map for this store
    //! \returns The named property map
    const name_id_map& get_name_id_map() const;
    //! \brief Get the shared database pointer used by this object
    //! \returns the shared_db_ptr
    shared_db_ptr get_db() const
        { return m_db; }

private:
    shared_db_ptr m_db;                             //!< The official shared_db_ptr used by this store
    mutable std::tr1::shared_ptr<property_bag> m_bag;    //!< The official property bag of this store object
    mutable std::tr1::shared_ptr<name_id_map> m_map;     //!< The official named property map of this store object
};

} // end pstsdk namespace

inline const pstsdk::property_bag& pstsdk::pst::get_property_bag() const
{
    if(!m_bag)
        m_bag.reset(new property_bag(m_db->lookup_node(nid_message_store)));

    return *m_bag;
}

inline pstsdk::property_bag& pstsdk::pst::get_property_bag()
{
    return const_cast<property_bag&>(const_cast<const pst*>(this)->get_property_bag());
}

inline const pstsdk::name_id_map& pstsdk::pst::get_name_id_map() const
{
    if(!m_map)
        m_map.reset(new name_id_map(m_db));

    return *m_map;
}

inline pstsdk::name_id_map& pstsdk::pst::get_name_id_map()
{
    return const_cast<name_id_map&>(const_cast<const pst*>(this)->get_name_id_map());
}

inline pstsdk::folder pstsdk::pst::open_folder(const std::wstring& name) const
{
    folder_iterator iter = std::find_if(folder_begin(), folder_end(), compiler_workarounds::folder_name_equal(name));

    if(iter != folder_end())
        return *iter;

    throw key_not_found<std::wstring>(name);
}

#endif
