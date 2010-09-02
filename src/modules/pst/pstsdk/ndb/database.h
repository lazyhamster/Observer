//! \file
//! \brief Database implementation
//! \author Terry Mahaffey
//!
//! Contains the db_context implementations for ANSI and Unicode stores
//! \ingroup ndb

#ifndef PSTSDK_NDB_DATABASE_H
#define PSTSDK_NDB_DATABASE_H

#include <fstream>
#include <memory>

#include "pstsdk/util/btree.h"
#include "pstsdk/util/errors.h"
#include "pstsdk/util/primitives.h"
#include "pstsdk/util/util.h"

#include "pstsdk/disk/disk.h"

#include "pstsdk/ndb/node.h"
#include "pstsdk/ndb/page.h"
#include "pstsdk/ndb/database_iface.h"

namespace pstsdk 
{ 

class node;

template<typename T> 
class database_impl;
typedef database_impl<ulonglong> large_pst;
typedef database_impl<ulong> small_pst;

//! \brief Open a db_context for the given file
//! \throws invalid_format if the file format is not understood
//! \throws runtime_error if an error occurs opening the file
//! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK) if the CRC of this header doesn't match
//! \param[in] filename The filename to open
//! \returns A shared_ptr to the opened context
//! \ingroup ndb_databaserelated
shared_db_ptr open_database(const std::wstring& filename);
//! \brief Try to open the given file as an ANSI store
//! \throws invalid_format if the file format is not ANSI
//! \throws runtime_error if an error occurs opening the file
//! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK) if the CRC of this header doesn't match
//! \param[in] filename The filename to open
//! \returns A shared_ptr to the opened context
//! \ingroup ndb_databaserelated
std::tr1::shared_ptr<small_pst> open_small_pst(const std::wstring& filename);
//! \brief Try to open the given file as a Unicode store
//! \throws invalid_format if the file format is not Unicode
//! \throws runtime_error if an error occurs opening the file
//! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK) if the CRC of this header doesn't match
//! \param[in] filename The filename to open
//! \returns A shared_ptr to the opened context
//! \ingroup ndb_databaserelated
std::tr1::shared_ptr<large_pst> open_large_pst(const std::wstring& filename);

//! \brief PST implementation
//!
//! The actual implementation of a database context - this class is responsible
//! for translating between the disk format (as indicated by the 
//! template parameter) and the disk-agnostic in memory classes returned from
//! the various factory methods.
//!
//! \ref open_database will instantiate the correct database_impl type for a
//! given filename.
//! \tparam T ulonglong for a Unicode store, ulong for an ANSI store
//! \ingroup ndb_databaserelated
template<typename T>
class database_impl : public db_context
{
public:

    //! \name Lookup functions
    //@{
    node lookup_node(node_id nid)
        { return node(this->shared_from_this(), lookup_node_info(nid)); }
    node_info lookup_node_info(node_id nid);
    block_info lookup_block_info(block_id bid); 
    //@}

    //! \name Page factory functions
    //@{
    std::tr1::shared_ptr<bbt_page> read_bbt_root();
    std::tr1::shared_ptr<nbt_page> read_nbt_root();
    std::tr1::shared_ptr<bbt_page> read_bbt_page(const page_info& pi);
    std::tr1::shared_ptr<nbt_page> read_nbt_page(const page_info& pi);
    std::tr1::shared_ptr<nbt_leaf_page> read_nbt_leaf_page(const page_info& pi);
    std::tr1::shared_ptr<bbt_leaf_page> read_bbt_leaf_page(const page_info& pi);
    std::tr1::shared_ptr<nbt_nonleaf_page> read_nbt_nonleaf_page(const page_info& pi);
    std::tr1::shared_ptr<bbt_nonleaf_page> read_bbt_nonleaf_page(const page_info& pi);
    //@}

    //! \name Block factory functions
    //@{
    std::tr1::shared_ptr<block> read_block(const shared_db_ptr& parent, block_id bid)
        { return read_block(parent, lookup_block_info(bid)); }
    std::tr1::shared_ptr<data_block> read_data_block(const shared_db_ptr& parent, block_id bid)
        { return read_data_block(parent, lookup_block_info(bid)); }
    std::tr1::shared_ptr<extended_block> read_extended_block(const shared_db_ptr& parent, block_id bid)
        { return read_extended_block(parent, lookup_block_info(bid)); }
    std::tr1::shared_ptr<external_block> read_external_block(const shared_db_ptr& parent, block_id bid)
        { return read_external_block(parent, lookup_block_info(bid)); }
    std::tr1::shared_ptr<subnode_block> read_subnode_block(const shared_db_ptr& parent, block_id bid)
        { return read_subnode_block(parent, lookup_block_info(bid)); }
    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, block_id bid)
        { return read_subnode_leaf_block(parent, lookup_block_info(bid)); }
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, block_id bid)
        { return read_subnode_nonleaf_block(parent, lookup_block_info(bid)); }

    std::tr1::shared_ptr<block> read_block(const shared_db_ptr& parent, const block_info& bi);
    std::tr1::shared_ptr<data_block> read_data_block(const shared_db_ptr& parent, const block_info& bi);
    std::tr1::shared_ptr<extended_block> read_extended_block(const shared_db_ptr& parent, const block_info& bi);
    std::tr1::shared_ptr<external_block> read_external_block(const shared_db_ptr& parent, const block_info& bi);
    std::tr1::shared_ptr<subnode_block> read_subnode_block(const shared_db_ptr& parent, const block_info& bi);
    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi);
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi);
    //@}

//! \cond write_api
    std::tr1::shared_ptr<external_block> create_external_block(const shared_db_ptr& parent, size_t size);
    std::tr1::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, std::tr1::shared_ptr<external_block>& pblock);
    std::tr1::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, std::tr1::shared_ptr<extended_block>& pblock);
    std::tr1::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, size_t size);

    block_id alloc_bid(bool is_internal);
//! \endcond

protected:
    database_impl(); // = delete
    //! \brief Construct a database_impl from this filename
    //! \throws invalid_format if the file format is not understood
    //! \throws runtime_error if an error occurs opening the file
    //! \param[in] filename The filename to open
    database_impl(const std::wstring& filename);
    //! \brief Validate the header of this file
    //! \throws invalid_format if this header is for a database format incompatible with this object
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK) if the CRC of this header doesn't match
    void validate_header();

    //! \brief Read block data, perform validation checks
    //! \param[in] bi The block information to read from disk
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The validated block data (still "encrypted")
    std::vector<byte> read_block_data(const block_info& bi);
    //! \brief Read page data, perform validation checks
    //! \param[in] pi The page information to read from disk
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The validated page data
    std::vector<byte> read_page_data(const page_info& pi);

    std::tr1::shared_ptr<nbt_leaf_page> read_nbt_leaf_page(const page_info& pi, disk::nbt_leaf_page<T>& the_page);
    std::tr1::shared_ptr<bbt_leaf_page> read_bbt_leaf_page(const page_info& pi, disk::bbt_leaf_page<T>& the_page);

    template<typename K, typename V>
    std::tr1::shared_ptr<bt_nonleaf_page<K,V> > read_bt_nonleaf_page(const page_info& pi, disk::bt_page<T, disk::bt_entry<T> >& the_page);

    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_leaf_block<T>& sub_block);
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_nonleaf_block<T>& sub_block);

    friend shared_db_ptr open_database(const std::wstring& filename);
    friend std::tr1::shared_ptr<small_pst> open_small_pst(const std::wstring& filename);
    friend std::tr1::shared_ptr<large_pst> open_large_pst(const std::wstring& filename);

    file m_file;
    disk::header<T> m_header;
    std::tr1::shared_ptr<bbt_page> m_bbt_root;
    std::tr1::shared_ptr<nbt_page> m_nbt_root;
};

//! \cond dont_show_these_member_function_specializations
template<>
inline void database_impl<ulong>::validate_header()
{
    // the behavior of open_database depends on this throw; this can not go under PSTSDK_VALIDATION_WEAK
    if(m_header.wVer >= disk::database_format_unicode_min)
        throw invalid_format();

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    ulong crc = disk::compute_crc(((byte*)&m_header) + disk::header_crc_locations<ulong>::start, disk::header_crc_locations<ulong>::length);

    if(crc != m_header.dwCRCPartial)
        throw crc_fail("header dwCRCPartial failure", 0, 0, crc, m_header.dwCRCPartial);
#endif
}

template<>
inline void database_impl<ulonglong>::validate_header()
{
    // the behavior of open_database depends on this throw; this can not go under PSTSDK_VALIDATION_WEAK
    if(m_header.wVer < disk::database_format_unicode_min)
        throw invalid_format();

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    ulong crc_partial = disk::compute_crc(((byte*)&m_header) + disk::header_crc_locations<ulonglong>::partial_start, disk::header_crc_locations<ulonglong>::partial_length);
    ulong crc_full = disk::compute_crc(((byte*)&m_header) + disk::header_crc_locations<ulonglong>::full_start, disk::header_crc_locations<ulonglong>::full_length);

    if(crc_partial != m_header.dwCRCPartial)
        throw crc_fail("header dwCRCPartial failure", 0, 0, crc_partial, m_header.dwCRCPartial);

    if(crc_full != m_header.dwCRCFull)
        throw crc_fail("header dwCRCFull failure", 0, 0, crc_full, m_header.dwCRCFull);
#endif
}
//! \endcond
} // end namespace

inline pstsdk::shared_db_ptr pstsdk::open_database(const std::wstring& filename)
{
    try 
    {
        shared_db_ptr db = open_small_pst(filename);
        return db;
    }
    catch(invalid_format&)
    {
        // well, that didn't work
    }

    shared_db_ptr db = open_large_pst(filename);
    return db;
}

inline std::tr1::shared_ptr<pstsdk::small_pst> pstsdk::open_small_pst(const std::wstring& filename)
{
    std::tr1::shared_ptr<small_pst> db(new small_pst(filename));
    return db;
}

inline std::tr1::shared_ptr<pstsdk::large_pst> pstsdk::open_large_pst(const std::wstring& filename)
{
    std::tr1::shared_ptr<large_pst> db(new large_pst(filename));
    return db;
}

template<typename T>
inline std::vector<pstsdk::byte> pstsdk::database_impl<T>::read_block_data(const block_info& bi)
{
    size_t aligned_size = disk::align_disk<T>(bi.size);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(aligned_size > disk::max_block_disk_size)
        throw unexpected_block("nonsensical block size");

    if(bi.address + aligned_size > m_header.root_info.ibFileEof)
        throw unexpected_block("nonsensical block location; past eof");
#endif

    std::vector<byte> buffer(aligned_size);
    disk::block_trailer<T>* bt = (disk::block_trailer<T>*)(&buffer[0] + aligned_size - sizeof(disk::block_trailer<T>));

    m_file.read(buffer, bi.address);    

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(bt->bid != bi.id)
        throw unexpected_block("wrong block id");

    if(bt->cb != bi.size)
        throw unexpected_block("wrong block size");

    if(bt->signature != disk::compute_signature(bi.id, bi.address))
        throw sig_mismatch("block sig mismatch", bi.address, bi.id, disk::compute_signature(bi.id, bi.address), bt->signature);
#endif

#ifdef PSTSDK_VALIDATION_LEVEL_FULL
    ulong crc = disk::compute_crc(&buffer[0], bi.size);
    if(crc != bt->crc)
        throw crc_fail("block crc failure", bi.address, bi.id, crc, bt->crc);
#endif

    return buffer;
}

template<typename T>
std::vector<pstsdk::byte> pstsdk::database_impl<T>::read_page_data(const page_info& pi)
{
#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(pi.address + disk::page_size > m_header.root_info.ibFileEof)
        throw unexpected_page("nonsensical page location; past eof");

    if(((pi.address - disk::first_amap_page_location) % disk::page_size) != 0)
        throw unexpected_page("nonsensical page location; not sector aligned");
#endif

    std::vector<byte> buffer(disk::page_size);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    m_file.read(buffer, pi.address);

#ifdef PSTSDK_VALIDATION_LEVEL_FULL
    ulong crc = disk::compute_crc(&buffer[0], disk::page<T>::page_data_size);
    if(crc != ppage->trailer.crc)
        throw crc_fail("page crc failure", pi.address, pi.id, crc, ppage->trailer.crc);
#endif

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(ppage->trailer.bid != pi.id)
        throw unexpected_page("wrong page id");

    if(ppage->trailer.page_type != ppage->trailer.page_type_repeat)
        throw database_corrupt("ptype != ptype repeat?");

    if(ppage->trailer.signature != disk::compute_signature(pi.id, pi.address))
        throw sig_mismatch("page sig mismatch", pi.address, pi.id, disk::compute_signature(pi.id, pi.address), ppage->trailer.signature);
#endif

    return buffer;
}


template<typename T>
inline std::tr1::shared_ptr<pstsdk::bbt_page> pstsdk::database_impl<T>::read_bbt_root()
{ 
    if(!m_bbt_root)
    {
        page_info pi = { m_header.root_info.brefBBT.bid, m_header.root_info.brefBBT.ib };
        m_bbt_root = read_bbt_page(pi); 
    }

    return m_bbt_root;
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::nbt_page> pstsdk::database_impl<T>::read_nbt_root()
{ 
    if(!m_nbt_root)
    {
        page_info pi = { m_header.root_info.brefNBT.bid, m_header.root_info.brefNBT.ib };
        m_nbt_root = read_nbt_page(pi);
    }

    return m_nbt_root;
}

template<typename T>
inline pstsdk::database_impl<T>::database_impl(const std::wstring& filename)
: m_file(filename)
{
    std::vector<byte> buffer(sizeof(m_header));
    m_file.read(buffer, 0);
    memcpy(&m_header, &buffer[0], sizeof(m_header));

    validate_header();
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::nbt_leaf_page> pstsdk::database_impl<T>::read_nbt_leaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_nbt)
    {
        disk::nbt_leaf_page<T>* leaf_page = (disk::nbt_leaf_page<T>*)ppage;

        if(leaf_page->level == 0)
            return read_nbt_leaf_page(pi, *leaf_page);
    }

    throw unexpected_page("page_type != page_type_nbt");
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::nbt_leaf_page> pstsdk::database_impl<T>::read_nbt_leaf_page(const page_info& pi, disk::nbt_leaf_page<T>& the_page)
{
    node_info ni;
    std::vector<std::pair<node_id, node_info> > nodes;

    for(int i = 0; i < the_page.num_entries; ++i)
    {
        ni.id = static_cast<node_id>(the_page.entries[i].nid);
        ni.data_bid = the_page.entries[i].data;
        ni.sub_bid = the_page.entries[i].sub;
        ni.parent_id = the_page.entries[i].parent_nid;

        nodes.push_back(std::make_pair(ni.id, ni));
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<nbt_leaf_page>(new nbt_leaf_page(shared_from_this(), pi, std::move(nodes)));
#else
    return std::tr1::shared_ptr<nbt_leaf_page>(new nbt_leaf_page(shared_from_this(), pi, nodes));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::bbt_leaf_page> pstsdk::database_impl<T>::read_bbt_leaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_bbt)
    {
        disk::bbt_leaf_page<T>* leaf_page = (disk::bbt_leaf_page<T>*)ppage;

        if(leaf_page->level == 0)
            return read_bbt_leaf_page(pi, *leaf_page);
    }

    throw unexpected_page("page_type != page_type_bbt");
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::bbt_leaf_page> pstsdk::database_impl<T>::read_bbt_leaf_page(const page_info& pi, disk::bbt_leaf_page<T>& the_page)
{
    block_info bi;
    std::vector<std::pair<block_id, block_info> > blocks;
    
    for(int i = 0; i < the_page.num_entries; ++i)
    {
        bi.id = the_page.entries[i].ref.bid;
        bi.address = the_page.entries[i].ref.ib;
        bi.size = the_page.entries[i].size;
        bi.ref_count = the_page.entries[i].ref_count;

        blocks.push_back(std::make_pair(bi.id, bi));
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<bbt_leaf_page>(new bbt_leaf_page(shared_from_this(), pi, std::move(blocks)));
#else
    return std::tr1::shared_ptr<bbt_leaf_page>(new bbt_leaf_page(shared_from_this(), pi, blocks));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::nbt_nonleaf_page> pstsdk::database_impl<T>::read_nbt_nonleaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_nbt)
    {
        disk::nbt_nonleaf_page<T>* nonleaf_page = (disk::nbt_nonleaf_page<T>*)ppage;

        if(nonleaf_page->level > 0)
            return read_bt_nonleaf_page<node_id, node_info>(pi, *nonleaf_page);
    }

    throw unexpected_page("page_type != page_type_nbt");
}

template<typename T>
template<typename K, typename V>
inline std::tr1::shared_ptr<pstsdk::bt_nonleaf_page<K,V> > pstsdk::database_impl<T>::read_bt_nonleaf_page(const page_info& pi, pstsdk::disk::bt_page<T, disk::bt_entry<T> >& the_page)
{
    std::vector<std::pair<K, page_info> > nodes;
    
    for(int i = 0; i < the_page.num_entries; ++i)
    {
        page_info subpi = { the_page.entries[i].ref.bid, the_page.entries[i].ref.ib };
        nodes.push_back(std::make_pair(static_cast<K>(the_page.entries[i].key), subpi));
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<bt_nonleaf_page<K,V> >(new bt_nonleaf_page<K,V>(shared_from_this(), pi, the_page.level, std::move(nodes)));
#else
    return std::tr1::shared_ptr<bt_nonleaf_page<K,V> >(new bt_nonleaf_page<K,V>(shared_from_this(), pi, the_page.level, nodes));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::bbt_nonleaf_page> pstsdk::database_impl<T>::read_bbt_nonleaf_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];
    
    if(ppage->trailer.page_type == disk::page_type_bbt)
    {
        disk::bbt_nonleaf_page<T>* nonleaf_page = (disk::bbt_nonleaf_page<T>*)ppage;

        if(nonleaf_page->level > 0)
            return read_bt_nonleaf_page<block_id, block_info>(pi, *nonleaf_page);
    }

    throw unexpected_page("page_type != page_type_bbt");
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::bbt_page> pstsdk::database_impl<T>::read_bbt_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];

    if(ppage->trailer.page_type == disk::page_type_bbt)
    {
        disk::bbt_leaf_page<T>* leaf = (disk::bbt_leaf_page<T>*)ppage;
        if(leaf->level == 0)
        {
            // it really is a leaf!
            return read_bbt_leaf_page(pi, *leaf);
        }
        else
        {
            disk::bbt_nonleaf_page<T>* nonleaf = (disk::bbt_nonleaf_page<T>*)ppage;
            return read_bt_nonleaf_page<block_id, block_info>(pi, *nonleaf);
        }
    }
    else
    {
        throw unexpected_page("page_type != page_type_bbt");
    }  
}
        
template<typename T>
inline std::tr1::shared_ptr<pstsdk::nbt_page> pstsdk::database_impl<T>::read_nbt_page(const page_info& pi)
{
    std::vector<byte> buffer = read_page_data(pi);
    disk::page<T>* ppage = (disk::page<T>*)&buffer[0];

    if(ppage->trailer.page_type == disk::page_type_nbt)
    {
        disk::nbt_leaf_page<T>* leaf = (disk::nbt_leaf_page<T>*)ppage;
        if(leaf->level == 0)
        {
            // it really is a leaf!
            return read_nbt_leaf_page(pi, *leaf);
        }
        else
        {
            disk::nbt_nonleaf_page<T>* nonleaf = (disk::nbt_nonleaf_page<T>*)ppage;
            return read_bt_nonleaf_page<node_id, node_info>(pi, *nonleaf);
        }
    }
    else
    {
        throw unexpected_page("page_type != page_type_nbt");
    }  
}

template<typename T>
inline pstsdk::node_info pstsdk::database_impl<T>::lookup_node_info(node_id nid)
{
    return read_nbt_root()->lookup(nid); 
}

template<typename T>
inline pstsdk::block_info pstsdk::database_impl<T>::lookup_block_info(block_id bid)
{
    if(bid == 0)
    {
        block_info bi;
        bi.id = bi.address = bi.size = bi.ref_count = 0;
        return bi;
    }
    else
    {
        return read_bbt_root()->lookup(bid & (~(block_id(disk::block_id_attached_bit))));
    }
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::block> pstsdk::database_impl<T>::read_block(const shared_db_ptr& parent, const block_info& bi)
{
    std::tr1::shared_ptr<block> pblock;

    try
    {
        pblock = read_data_block(parent, bi);
    }
    catch(unexpected_block&)
    {
        pblock = read_subnode_block(parent, bi);
    }

    return pblock;
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::data_block> pstsdk::database_impl<T>::read_data_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(disk::bid_is_external(bi.id))
        return read_external_block(parent, bi);

    std::vector<byte> buffer(sizeof(disk::extended_block<T>));
    disk::extended_block<T>* peblock = (disk::extended_block<T>*)&buffer[0];
    m_file.read(buffer, bi.address);

    // the behavior of read_block depends on this throw; this can not go under PSTSDK_VALIDATION_WEAK
    if(peblock->block_type != disk::block_type_extended)
        throw unexpected_block("extended block expected");

    return read_extended_block(parent, bi);
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::extended_block> pstsdk::database_impl<T>::read_extended_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(!disk::bid_is_internal(bi.id))
        throw unexpected_block("internal bid expected");

    std::vector<byte> buffer = read_block_data(bi);
    disk::extended_block<T>* peblock = (disk::extended_block<T>*)&buffer[0];
    std::vector<block_id> child_blocks;

    for(int i = 0; i < peblock->count; ++i)
        child_blocks.push_back(peblock->bid[i]);

#ifdef __GNUC__
    // GCC gave link errors on extended_block<T> and external_block<T> max_size
    // with the below alernative
    uint sub_size = 0;
    if(peblock->level == 1)
        sub_size = disk::external_block<T>::max_size;
    else
        sub_size = disk::extended_block<T>::max_size;
#else
    uint sub_size = (peblock->level == 1 ? disk::external_block<T>::max_size : disk::extended_block<T>::max_size);
#endif
    uint sub_page_count = peblock->level == 1 ? 1 : disk::extended_block<T>::max_count;

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, bi, peblock->level, peblock->total_size, sub_size, disk::extended_block<T>::max_count, sub_page_count, std::move(child_blocks)));
#else
    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, bi, peblock->level, peblock->total_size, sub_size, disk::extended_block<T>::max_count, sub_page_count, child_blocks));
#endif
}

//! \cond write_api
template<typename T>
inline std::tr1::shared_ptr<pstsdk::external_block> pstsdk::database_impl<T>::create_external_block(const shared_db_ptr& parent, size_t size)
{
    return std::tr1::shared_ptr<external_block>(new external_block(parent, disk::external_block<T>::max_size, size));
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::extended_block> pstsdk::database_impl<T>::create_extended_block(const shared_db_ptr& parent, std::tr1::shared_ptr<external_block>& pchild_block)
{
    std::vector<std::tr1::shared_ptr<data_block> > child_blocks;
    child_blocks.push_back(pchild_block);

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, 1, pchild_block->get_total_size(), disk::external_block<T>::max_size, disk::extended_block<T>::max_count, 1, std::move(child_blocks)));
#else
    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, 1, pchild_block->get_total_size(), disk::external_block<T>::max_size, disk::extended_block<T>::max_count, 1, child_blocks));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::extended_block> pstsdk::database_impl<T>::create_extended_block(const shared_db_ptr& parent, std::tr1::shared_ptr<extended_block>& pchild_block)
{
    std::vector<std::tr1::shared_ptr<data_block> > child_blocks;
    child_blocks.push_back(pchild_block);

    assert(pchild_block->get_level() == 1);

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, 2, pchild_block->get_total_size(), disk::extended_block<T>::max_size, disk::extended_block<T>::max_count, disk::extended_block<T>::max_count, std::move(child_blocks)));
#else
    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, 2, pchild_block->get_total_size(), disk::extended_block<T>::max_size, disk::extended_block<T>::max_count, disk::extended_block<T>::max_count, child_blocks));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::extended_block> pstsdk::database_impl<T>::create_extended_block(const shared_db_ptr& parent, size_t size)
{
    ushort level = size > disk::extended_block<T>::max_size ? 2 : 1;
#ifdef __GNUC__
    // More strange link errors
    size_t child_max_size;
    if(level == 1)
        child_max_size = disk::external_block<T>::max_size;
    else
        child_max_size = disk::extended_block<T>::max_size;
#else
    size_t child_max_size = level == 1 ? disk::external_block<T>::max_size : disk::extended_block<T>::max_size;
#endif
    ulong child_max_blocks = level == 1 ? 1 : disk::extended_block<T>::max_count;

    return std::tr1::shared_ptr<extended_block>(new extended_block(parent, level, size, child_max_size, disk::extended_block<T>::max_count, child_max_blocks));
}
//! \endcond

template<typename T>
inline std::tr1::shared_ptr<pstsdk::external_block> pstsdk::database_impl<T>::read_external_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(bi.id == 0)
    {
        return std::tr1::shared_ptr<external_block>(new external_block(parent, bi, disk::external_block<T>::max_size,  std::vector<byte>()));
    }

    if(!disk::bid_is_external(bi.id))
        throw unexpected_block("External BID expected");

    std::vector<byte> buffer = read_block_data(bi);

    if(m_header.bCryptMethod == disk::crypt_method_permute)
    {
        disk::permute(&buffer[0], bi.size, false);
    }
    else if(m_header.bCryptMethod == disk::crypt_method_cyclic)
    {
        disk::cyclic(&buffer[0], bi.size, (ulong)bi.id);
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<external_block>(new external_block(parent, bi, disk::external_block<T>::max_size, std::move(buffer)));
#else
    return std::tr1::shared_ptr<external_block>(new external_block(parent, bi, disk::external_block<T>::max_size, buffer));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::subnode_block> pstsdk::database_impl<T>::read_subnode_block(const shared_db_ptr& parent, const block_info& bi)
{
    if(bi.id == 0)
    {
        return std::tr1::shared_ptr<subnode_block>(new subnode_leaf_block(parent, bi, std::vector<std::pair<node_id, subnode_info> >()));
    }
    
    std::vector<byte> buffer = read_block_data(bi);
    disk::sub_leaf_block<T>* psub = (disk::sub_leaf_block<T>*)&buffer[0];
    std::tr1::shared_ptr<subnode_block> sub_block;

    if(psub->level == 0)
    {
        sub_block = read_subnode_leaf_block(parent, bi, *psub);
    }
    else
    {
        sub_block = read_subnode_nonleaf_block(parent, bi, *(disk::sub_nonleaf_block<T>*)&buffer[0]);
    }

    return sub_block;
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::subnode_leaf_block> pstsdk::database_impl<T>::read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi)
{
    std::vector<byte> buffer = read_block_data(bi);
    disk::sub_leaf_block<T>* psub = (disk::sub_leaf_block<T>*)&buffer[0];
    std::tr1::shared_ptr<subnode_leaf_block> sub_block;

    if(psub->level == 0)
    {
        sub_block = read_subnode_leaf_block(parent, bi, *psub);
    }
    else
    {
        throw unexpected_block("psub->level != 0");
    }

    return sub_block; 
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::subnode_nonleaf_block> pstsdk::database_impl<T>::read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi)
{
    std::vector<byte> buffer = read_block_data(bi);
    disk::sub_nonleaf_block<T>* psub = (disk::sub_nonleaf_block<T>*)&buffer[0];
    std::tr1::shared_ptr<subnode_nonleaf_block> sub_block;

    if(psub->level != 0)
    {
        sub_block = read_subnode_nonleaf_block(parent, bi, *psub);
    }
    else
    {
        throw unexpected_block("psub->level == 1");
    }

    return sub_block;
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::subnode_leaf_block> pstsdk::database_impl<T>::read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_leaf_block<T>& sub_block)
{
    subnode_info ni;
    std::vector<std::pair<node_id, subnode_info> > subnodes;

    for(int i = 0; i < sub_block.count; ++i)
    {
        ni.id = sub_block.entry[i].nid;
        ni.data_bid = sub_block.entry[i].data;
        ni.sub_bid = sub_block.entry[i].sub;

        subnodes.push_back(std::make_pair(sub_block.entry[i].nid, ni));
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<subnode_leaf_block>(new subnode_leaf_block(parent, bi, std::move(subnodes)));
#else
    return std::tr1::shared_ptr<subnode_leaf_block>(new subnode_leaf_block(parent, bi, subnodes));
#endif
}

template<typename T>
inline std::tr1::shared_ptr<pstsdk::subnode_nonleaf_block> pstsdk::database_impl<T>::read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi, disk::sub_nonleaf_block<T>& sub_block)
{
    std::vector<std::pair<node_id, block_id> > subnodes;

    for(int i = 0; i < sub_block.count; ++i)
    {
        subnodes.push_back(std::make_pair(sub_block.entry[i].nid_key, sub_block.entry[i].sub_block_bid));
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<subnode_nonleaf_block>(new subnode_nonleaf_block(parent, bi, std::move(subnodes)));
#else
    return std::tr1::shared_ptr<subnode_nonleaf_block>(new subnode_nonleaf_block(parent, bi, subnodes));
#endif
}

//! \cond write_api
template<typename T>
inline pstsdk::block_id pstsdk::database_impl<T>::alloc_bid(bool is_internal)
{
#ifdef __GNUC__
    typename disk::header<T>::block_id_disk disk_id;
    memcpy(&disk_id, m_header.bidNextB, sizeof(disk_id));

    block_id next_bid = disk_id;

    disk_id += disk::block_id_increment;
    memcpy(m_header.bidNextB, &disk_id, sizeof(disk_id));
#else
    block_id next_bid = m_header.bidNextB;
    m_header.bidNextB += disk::block_id_increment;
#endif


    if(is_internal)
        next_bid &= disk::block_id_internal_bit;

    return next_bid;
}
//! \endcond

#endif
