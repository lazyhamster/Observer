//! \file
//! \brief Page definitions
//! \author Terry Mahaffey
//!
//! A page is 512 bytes of metadata contained in a PST file. Pages come in
//! two general flavors:
//! - BTree Pages (immutable)
//! - Allocation Related Pages (mutable)
//!
//! The former are pages which collectively form the BBT and NBT. They are
//! immutable. The later exist at predefined locations in the file, and are
//! always modified in place.
//! \ingroup ndb

#ifndef PSTSDK_NDB_PAGE_H
#define PSTSDK_NDB_PAGE_H

#include <vector>

#include "pstsdk/util/btree.h"
#include "pstsdk/util/util.h"

#include "pstsdk/ndb/database_iface.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4250)
#endif

namespace pstsdk
{

//! \defgroup ndb_pagerelated Pages
//! \ingroup ndb

//! \brief Generic base class for all page types
//!
//! This class provides an abstraction around the page trailer located at the
//! end of every page. The actual page content is interpretted by the child
//! classes of \ref page.
//!
//! All pages in the pages hierarchy are also in the category of what is known
//! as <i>dependant objects</i>. This means is they only keep a weak 
//! reference to the database context to which they're a member. Contrast this
//! to an independant object such as the \ref heap, which keeps a strong ref
//! or a full shared_ptr to the related context. This implies that someone
//! must externally make sure the database context outlives it's pages -
//! this is usually done by the database context itself.
//! \sa [MS-PST] 2.2.2.7
//! \ingroup ndb_pagerelated
class page
{
public:
    //! \brief Construct a page from disk
    //! \param[in] db The database context
    //! \param[in] pi Information about this page
    page(const shared_db_ptr& db, const page_info& pi)
        : m_db(db), m_pid(pi.id), m_address(pi.address) { }

    //! \brief Get the page id
    //! \returns The page id
    page_id get_page_id() const { return m_pid; }

    //! \brief Get the physical address of this page
    //! \returns The address of this page, or 0 for a new page
    ulonglong get_address() const { return m_address; }

protected:
    shared_db_ptr get_db_ptr() const { return shared_db_ptr(m_db); }
    weak_db_ptr m_db;       //!< The database context we're a member of
    page_id m_pid;          //!< Page id
    ulonglong m_address;    //!< Address of this page
};

//!< \brief A page which forms a node in the NBT or BBT
//!
//! The NBT and BBT form the core of the internals of the PST, and need to be
//! well understood if working at the \ref ndb. The bt_page class forms the
//! nodes of both the NBT and BBT, with child classes for leaf and nonleaf
//! pages.
//!
//! This hierarchy also models the \ref btree_node structure, inheriting the 
//! actual iteration and lookup logic.
//! \tparam K key type
//! \tparam V value type
//! \sa [MS-PST] 1.3.1.1
//! \sa [MS-PST] 2.2.2.7.7
//! \ingroup ndb_pagerelated
template<typename K, typename V>
class bt_page : 
    public page, 
    public virtual btree_node<K,V>
{
public:
    //! \brief Construct a bt_page from disk
    //! \param[in] db The database context
    //! \param[in] pi Information about this page
    //! \param[in] level 0 for a leaf, or distance from leaf
    bt_page(const shared_db_ptr& db, const page_info& pi, ushort level)
        : page(db, pi), m_level(level) { }

    //! \brief Returns the level of this bt_page
    //! \returns The level of this page
    ushort get_level() const { return m_level; }

private:
    ushort m_level; //!< Our level
};

//! \brief Contains references to other bt_pages
//!
//! A bt_nonleaf_page makes up the body of the NBT and BBT (which differ only
//! at the leaf). 
//! \tparam K key type
//! \tparam V value type
//! \sa [MS-PST] 2.2.2.7.7.2
//! \ingroup ndb_pagerelated
template<typename K, typename V>
class bt_nonleaf_page : 
    public bt_page<K,V>, 
    public btree_node_nonleaf<K,V>, 
    public std::tr1::enable_shared_from_this<bt_nonleaf_page<K,V> >
{
public:
    //! \brief Construct a bt_nonleaf_page from disk
    //! \param[in] db The database context
    //! \param[in] pi Information about this page
    //! \param[in] level Distance from leaf
    //! \param[in] subpi Information about the child pages
#ifndef BOOST_NO_RVALUE_REFERENCES
    bt_nonleaf_page(const shared_db_ptr& db, const page_info& pi, ushort level, std::vector<std::pair<K, page_info> > subpi)
        : bt_page<K,V>(db, pi, level), m_page_info(std::move(subpi)), m_child_pages(m_page_info.size()) { }
#else
    bt_nonleaf_page(const shared_db_ptr& db, const page_info& pi, ushort level, const std::vector<std::pair<K, page_info> >& subpi)
        : bt_page<K,V>(db, pi, level), m_page_info(subpi), m_child_pages(m_page_info.size()) { }
#endif

    // btree_node_nonleaf implementation
    const K& get_key(uint pos) const { return m_page_info[pos].first; }
    bt_page<K,V>* get_child(uint pos);
    const bt_page<K,V>* get_child(uint pos) const;
    uint num_values() const { return m_child_pages.size(); }

private:
    std::vector<std::pair<K, page_info> > m_page_info;   //!< Information about the child pages
    mutable std::vector<std::tr1::shared_ptr<bt_page<K,V> > > m_child_pages; //!< Cached child pages
};

//! \brief Contains the actual key value pairs of the btree
//! \tparam K key type
//! \tparam V value type
//! \ingroup ndb_pagerelated
template<typename K, typename V>
class bt_leaf_page : 
    public bt_page<K,V>, 
    public btree_node_leaf<K,V>, 
    public std::tr1::enable_shared_from_this<bt_leaf_page<K,V> >
{
public:
    //! \brief Construct a leaf page from disk
    //! \param[in] db The database context
    //! \param[in] pi Information about this page
    //! \param[in] data The key/value pairs on this leaf page
#ifndef BOOST_NO_RVALUE_REFERENCES
    bt_leaf_page(const shared_db_ptr& db, const page_info& pi, std::vector<std::pair<K,V> > data)
        : bt_page<K,V>(db, pi, 0), m_page_data(std::move(data)) { }
#else
    bt_leaf_page(const shared_db_ptr& db, const page_info& pi, const std::vector<std::pair<K,V> >& data)
        : bt_page<K,V>(db, pi, 0), m_page_data(data) { }
#endif

    // btree_node_leaf implementation
    const V& get_value(uint pos) const
        { return m_page_data[pos].second; }
    const K& get_key(uint pos) const
        { return m_page_data[pos].first; }
    uint num_values() const
        { return m_page_data.size(); }

private:
    std::vector<std::pair<K,V> > m_page_data; //!< The key/value pairs on this leaf page
};
//! \cond dont_show_these_member_function_specializations
template<>
inline bt_page<block_id, block_info>* bt_nonleaf_page<block_id, block_info>::get_child(uint pos)
{
    if(m_child_pages[pos] == NULL)
    {
        m_child_pages[pos] = this->get_db_ptr()->read_bbt_page(m_page_info[pos].second);
    }

    return m_child_pages[pos].get();
}

template<>
inline const bt_page<block_id, block_info>* bt_nonleaf_page<block_id, block_info>::get_child(uint pos) const
{
    if(m_child_pages[pos] == NULL)
    {
        m_child_pages[pos] = this->get_db_ptr()->read_bbt_page(m_page_info[pos].second);
    }

    return m_child_pages[pos].get();
}

template<>
inline bt_page<node_id, node_info>* bt_nonleaf_page<node_id, node_info>::get_child(uint pos)
{
    if(m_child_pages[pos] == NULL)
    {
        m_child_pages[pos] = this->get_db_ptr()->read_nbt_page(m_page_info[pos].second); 
    }

    return m_child_pages[pos].get();
}

template<>
inline const bt_page<node_id, node_info>* bt_nonleaf_page<node_id, node_info>::get_child(uint pos) const
{
    if(m_child_pages[pos] == NULL)
    {
        m_child_pages[pos] = this->get_db_ptr()->read_nbt_page(m_page_info[pos].second); 
    }

    return m_child_pages[pos].get();
}
//! \endcond
} // end namespace

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
