//! \file
//! \brief Database interface
//! \author Terry Mahaffey
//!
//! Contains the db_context interface as well as some broadly used primitive 
//! in memory types and typedefs.
//! \ingroup ndb

#ifndef PSTSDK_NDB_DATABASE_IFACE_H
#define PSTSDK_NDB_DATABASE_IFACE_H

#include <memory>
#ifdef __GNUC__
#include <tr1/memory>
#endif

#include "pstsdk/util/util.h"
#include "pstsdk/util/primitives.h"

namespace pstsdk
{

class node;

//! \brief An in memory, database format agnostic version of \ref disk::bbt_leaf_entry.
//!
//! It doesn't contain a ref count because that value is of no interest to the
//! in memory objects.
//! \ingroup ndb
struct block_info
{
    block_id id;
    ulonglong address;
    ushort size;
    ushort ref_count;
};

//! \brief An in memory, database format agnostic version of \ref disk::block_reference
//! used specifically for the \ref page class hierarchy
//! \ingroup ndb
struct page_info
{
    page_id id;
    ulonglong address;
};

//! \brief An in memory, database format agnostic version of \ref disk::nbt_leaf_entry.
//! \ingroup ndb
struct node_info
{
    node_id id;
    block_id data_bid;
    block_id sub_bid;
    node_id parent_id;
};

//! \brief An in memory, database format agnostic version of \ref disk::sub_leaf_entry.
//! \ingroup ndb
struct subnode_info
{
    node_id id;
    block_id data_bid;
    block_id sub_bid;
};

template<typename K, typename V>
class bt_page;
typedef bt_page<node_id, node_info> nbt_page;
typedef bt_page<block_id, block_info> bbt_page;

template<typename K, typename V>
class bt_nonleaf_page;
typedef bt_nonleaf_page<node_id, node_info> nbt_nonleaf_page;
typedef bt_nonleaf_page<block_id, block_info> bbt_nonleaf_page;

template<typename K, typename V>
class bt_leaf_page;
typedef bt_leaf_page<node_id, node_info> nbt_leaf_page;
typedef bt_leaf_page<block_id, block_info> bbt_leaf_page;

template<typename K, typename V>
class const_btree_node_iter;

//! \addtogroup ndb
//@{
typedef const_btree_node_iter<node_id, node_info> const_nodeinfo_iterator;
typedef const_btree_node_iter<node_id, subnode_info> const_subnodeinfo_iterator;
typedef const_btree_node_iter<block_id, block_info> const_blockinfo_iterator;
//@}

class block;
class data_block;
class extended_block;
class external_block;
class subnode_block;
class subnode_leaf_block;
class subnode_nonleaf_block;

class db_context;

//! \addtogroup ndb
//@{
typedef std::tr1::shared_ptr<db_context> shared_db_ptr;
typedef std::tr1::weak_ptr<db_context> weak_db_ptr;
//@}

//! \defgroup ndb_databaserelated Database
//! \ingroup ndb

//! \brief Database external interface
//!
//! The db_context is the iterface which all components, \ref ndb and up,
//! use to access the PST file. It is basically an object factory; given an
//! address (or other relative context to help locate the physical piece of
//! data) a db_context will produce an in memory version of that data
//! structure, with all the Unicode vs. ANSI differences abstracted away.
//! \ingroup ndb_databaserelated
class db_context : public std::tr1::enable_shared_from_this<db_context>
{
public:
    virtual ~db_context() { }

    //! \name Lookup functions
    //@{
    //! \brief Open a node
    //! \throws key_not_found<node_id> if the specified node was not found
    //! \param[in] nid The id of the node to lookup
    //! \returns A node instance
    virtual node lookup_node(node_id nid) = 0;
    //! \brief Lookup information about a node
    //! \throws key_not_found<node_id> if the specified node was not found
    //! \param[in] nid The id of the node to lookup
    //! \returns Information about the specified node
    virtual node_info lookup_node_info(node_id nid) = 0;
    //! \brief Lookup information about a block
    //! \throws key_not_found<block_id> if the specified block was not found
    //! \param[in] bid The id of the block to lookup
    //! \returns Information about the specified block
    virtual block_info lookup_block_info(block_id bid) = 0;
    //@}

    //! \name Page factory functions
    //@{
    //! \brief Get the root of the BBT of this context
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<bbt_page> read_bbt_root() = 0;
    //! \brief Get the root of the NBT of this context
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<nbt_page> read_nbt_root() = 0;
    //! \brief Open a BBT page
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<bbt_page> read_bbt_page(const page_info& pi) = 0;
    //! \brief Open a NBT page
    //! \param[in] pi Information about the page to open
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<nbt_page> read_nbt_page(const page_info& pi) = 0;
    //! \brief Open a NBT leaf page
    //! \param[in] pi Information about the page to open
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<nbt_leaf_page> read_nbt_leaf_page(const page_info& pi) = 0;
    //! \brief Open a BBT leaf page
    //! \param[in] pi Information about the page to open
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<bbt_leaf_page> read_bbt_leaf_page(const page_info& pi) = 0;
    //! \brief Open a NBT nonleaf page
    //! \param[in] pi Information about the page to open
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<nbt_nonleaf_page> read_nbt_nonleaf_page(const page_info& pi) = 0;
    //! \brief Open a BBT nonleaf page
    //! \param[in] pi Information about the page to open
    //! \throws unexpected_page (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the page appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's signature appears incorrect
    //! \throws database_corrupt (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page trailer's ptypeRepeat != ptype
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the page's CRC doesn't match the trailer
    //! \returns The requested page
    virtual std::tr1::shared_ptr<bbt_nonleaf_page> read_bbt_nonleaf_page(const page_info& pi) = 0;
    //@}

    //! \name Block factory functions
    //@{
    //! \brief Open a block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<block> read_block(block_id bid) { return read_block(shared_from_this(), bid); }
    //! \brief Open a data_block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<data_block> read_data_block(block_id bid) { return read_data_block(shared_from_this(), bid); }
    //! \brief Open a extended_block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<extended_block> read_extended_block(block_id bid) { return read_extended_block(shared_from_this(), bid); }
    //! \brief Open a external_block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<external_block> read_external_block(block_id bid) { return read_external_block(shared_from_this(), bid); }
    //! \brief Open a subnode_block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<subnode_block> read_subnode_block(block_id bid) { return read_subnode_block(shared_from_this(), bid); }
    //! \brief Open a subnode_leaf_block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(block_id bid) { return read_subnode_leaf_block(shared_from_this(), bid); }
    //! \brief Open a subnode_nonleaf_block in this context
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(block_id bid) { return read_subnode_nonleaf_block(shared_from_this(), bid); }
    //! \brief Open a block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<block> read_block(const shared_db_ptr& parent, block_id bid) = 0;
    //! \brief Open a data_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<data_block> read_data_block(const shared_db_ptr& parent, block_id bid) = 0;
    //! \brief Open an extended_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<extended_block> read_extended_block(const shared_db_ptr& parent, block_id bid) = 0;
    //! \brief Open a external_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<external_block> read_external_block(const shared_db_ptr& parent, block_id bid) = 0;
    //! \brief Open a subnode_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<subnode_block> read_subnode_block(const shared_db_ptr& parent, block_id bid) = 0;
    //! \brief Open a subnode_leaf_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, block_id bid) = 0;
    //! \brief Open a subnode_nonleaf_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bid The id of the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, block_id bid) = 0;

    //! \brief Open a block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<block> read_block(const block_info& bi) { return read_block(shared_from_this(), bi); }
    //! \brief Open a data_block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<data_block> read_data_block(const block_info& bi) { return read_data_block(shared_from_this(), bi); }
    //! \brief Open a extended_block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<extended_block> read_extended_block(const block_info& bi) { return read_extended_block(shared_from_this(), bi); }
    //! \brief Open a block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<external_block> read_external_block(const block_info& bi) { return read_external_block(shared_from_this(), bi); }
    //! \brief Open a subnode_block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<subnode_block> read_subnode_block(const block_info& bi) { return read_subnode_block(shared_from_this(), bi); }
    //! \brief Open a subnode_leaf_block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const block_info& bi) { return read_subnode_leaf_block(shared_from_this(), bi); }
    //! \brief Open a subnode_nonleaf_block in this context
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const block_info& bi) { return read_subnode_nonleaf_block(shared_from_this(), bi); }
    //! \brief Open a block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<block> read_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //! \brief Open a data_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<data_block> read_data_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //! \brief Open a extended_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<extended_block> read_extended_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //! \brief Open a external_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<external_block> read_external_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //! \brief Open a subnode_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<subnode_block> read_subnode_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //! \brief Open a subnode_leaf_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //! \brief Open a subnode_nonleaf_block in the specified context
    //! \param[in] parent The context to open this block in. It must be either this context or a child context of this context.
    //! \param[in] bi Information about the block to open
    //! \throws unexpected_block (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the parameters of the block appear incorrect
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the block trailer's signature appears incorrect
    //! \throws crc_fail (\ref PSTSDK_VALIDATION_LEVEL_WEAK "PSTSDK_VALIDATION_LEVEL_FULL") If the block's CRC doesn't match the trailer
    //! \returns The requested block
    virtual std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const shared_db_ptr& parent, const block_info& bi) = 0;
    //@}

//! \cond write_api
    std::tr1::shared_ptr<external_block> create_external_block(size_t size) { return create_external_block(shared_from_this(), size); }
    std::tr1::shared_ptr<extended_block> create_extended_block(std::tr1::shared_ptr<external_block>& pblock) { return create_extended_block(shared_from_this(), pblock); }
    std::tr1::shared_ptr<extended_block> create_extended_block(std::tr1::shared_ptr<extended_block>& pblock) { return create_extended_block(shared_from_this(), pblock); }
    std::tr1::shared_ptr<extended_block> create_extended_block(size_t size) { return create_extended_block(shared_from_this(), size); }
    virtual std::tr1::shared_ptr<external_block> create_external_block(const shared_db_ptr& parent, size_t size) = 0;
    virtual std::tr1::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, std::tr1::shared_ptr<external_block>& pblock) = 0;
    virtual std::tr1::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, std::tr1::shared_ptr<extended_block>& pblock) = 0;
    virtual std::tr1::shared_ptr<extended_block> create_extended_block(const shared_db_ptr& parent, size_t size) = 0;

    // header functions
    virtual block_id alloc_bid(bool is_internal) = 0;
//! \endcond
    // context support
    //virtual shared_db_ptr create_context() = 0;
    //virtual void save(const shared_db_ptr& ctx) = 0;
};

}

#endif

