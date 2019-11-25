//! \cond write_api
#ifndef PSTSDK_NDB_CONTEXT_H
#define PSTSDK_NDB_CONTEXT_H

#include <map>

#include "pstsdk/ndb/node.h"
#include "pstsdk/ndb/database_iface.h"

namespace pstsdk
{

class db_context_impl : public db_context
{
public:
    db_context_impl(const shared_db_ptr& parent)
        : m_db(parent), m_bbt_root(parent->read_bbt_root(shared_from_this())), m_nbt_root(parent->read_nbt_root(shared_from_this())) { }

    // database implementation
    node lookup_node(node_id nid)
        { return node(shared_from_this(), lookup_node_info(nid)); }
    node_info lookup_node_info(node_id nid);
    block_info lookup_block_info(block_id bid); 
    std::tr1::shared_ptr<bbt_page> read_bbt_root() { return m_bbt_root; }
    std::tr1::shared_ptr<nbt_page> read_nbt_root() { return m_nbt_root; }
    std::tr1::shared_ptr<bbt_page> read_bbt_page(ulonglong location) 
        { return m_db->read_bbt_page(shared_from_this(), location); }
    std::tr1::shared_ptr<nbt_page> read_nbt_page(ulonglong location) 
        { return m_db->read_nbt_page(shared_from_this(), location); }
    std::tr1::shared_ptr<nbt_leaf_page> read_nbt_leaf_page(ulonglong location) 
        { return m_db->read_nbt_leaf_page(shared_from_this(), location); }
    std::tr1::shared_ptr<bbt_leaf_page> read_bbt_leaf_page(ulonglong location) 
        { return m_db->read_bbt_leaf_page(shared_from_this(), location); }
    std::tr1::shared_ptr<nbt_nonleaf_page> read_nbt_nonleaf_page(ulonglong location) 
        { return m_db->read_nbt_nonleaf_page(shared_from_this(), location); }
    std::tr1::shared_ptr<bbt_nonleaf_page> read_bbt_nonleaf_page(ulonglong location)
        { return m_db->read_bbt_nonleaf_page(shared_from_this(), location); }
    std::tr1::shared_ptr<block> read_block(block_id bid);
    std::tr1::shared_ptr<data_block> read_data_block(block_id bid);
    std::tr1::shared_ptr<extended_block> read_extended_block(block_id bid);
    std::tr1::shared_ptr<external_block> read_external_block(block_id bid);
    std::tr1::shared_ptr<subnode_block> read_subnode_block(block_id bid);
    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(block_id bid);
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(block_id bid);
    
    std::tr1::shared_ptr<block> read_block(const block_info& bi);
    std::tr1::shared_ptr<data_block> read_data_block(const block_info& bi);
    std::tr1::shared_ptr<extended_block> read_extended_block(const block_info& bi);
    std::tr1::shared_ptr<external_block> read_external_block(const block_info& bi);
    std::tr1::shared_ptr<subnode_block> read_subnode_block(const block_info& bi);
    std::tr1::shared_ptr<subnode_leaf_block> read_subnode_leaf_block(const block_info& bi);
    std::tr1::shared_ptr<subnode_nonleaf_block> read_subnode_nonleaf_block(const block_info& bi);
private:
    shared_db_ptr m_db;
    std::tr1::shared_ptr<bbt_page> m_bbt_root;
    block_id m_base_bid;
    std::tr1::shared_ptr<nbt_page> m_nbt_root;
    node_id m_base_node_id;

    std::map<node_id, node_info> m_nodes; // nodes commited to this context
    std::vector<node_id> m_deleted_nodes; // nodes deleted in this context
    std::map<block_id, std::tr1::shared_ptr<block>> m_blocks; // blocks commited to this context (null db pointers)

    template<typename T>
    bool lookup_saved_block(block_id bid, std::tr1::shared_ptr<T>& pcached_block);
    template<typename T>
    bool add_saved_block(const std::tr1::shared_ptr<T>& pblock);
};

} // end pstsdk namespace

template<typename T>
bool pstsdk::db_context_impl::lookup_saved_block(block_id bid, std::tr1::shared_ptr<T>& pcached_block)
{
    auto pos = m_blocks.find(bid);
    if(pos != m_blocks.end())
    {
        std::tr1::shared_ptr<T> pcopy(new T(*std::static_pointer_cast<T>(pos->second)));
        static_pointer_cast<block>(pcopy)->set_db_ptr(shared_from_this());
        pcached_block = pcopy;
        return true;
    }
    return false;
}

template<typename T>
bool pstsdk::db_context_impl::add_saved_block(const std::tr1::shared_ptr<T>& pblock)
{
    auto pos = m_blocks.find(pblock->get_id());
    if(pos == m_blocks.end())
    {
        std::tr1::shared_ptr<block> pcopy(new T(*pblock));
        pcopy->set_db_ptr(shared_db_ptr());
        m_blocks[pcopy->get_id()] = pcopy;
        return true;
    }
    return false;
}

pstsdk::node_info pstsdk::db_context_impl::lookup_node_info(node_id nid)
{
    auto pos = m_nodes.find(nid);
    if(pos != m_nodes.end())
        return pos->second;

    return read_nbt_root()->lookup_node_info(nid);
}

pstsdk::block_info pstsdk::db_context_impl::lookup_block_info(block_id bid)
{
    std::tr1::shared_ptr<block> pblock;
    if(lookup_cached_block(bid, pblock))
    {
        block_info bi;
        bi.address = 0;
        bi.ref_count = 0;
        bi.id = pblock->get_id();
        bi.size = pblock->get_disk_size();
        return bi;
    }

    return read_bbt_root()->lookup_block_info(bid);
}

std::tr1::shared_ptr<pstsdk::block> pstsdk::db_context_impl::read_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_block(shared_from_this(), lookup_block_info(bid));
}

std::tr1::shared_ptr<pstsdk::data_block> pstsdk::db_context_impl::read_data_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::data_block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_data_block(shared_from_this(), lookup_block_info(bid));
}

std::tr1::shared_ptr<pstsdk::extended_block> pstsdk::db_context_impl::read_extended_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::extended_block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_extended_block(shared_from_this(), lookup_block_info(bid));
}

std::tr1::shared_ptr<pstsdk::external_block> pstsdk::db_context_impl::read_external_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::external_block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_external_block(shared_from_this(), lookup_block_info(bid));
}

std::tr1::shared_ptr<pstsdk::subnode_block> pstsdk::db_context_impl::read_subnode_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::subnode_block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_subnode_block(shared_from_this(), lookup_block_info(bid));
}

std::tr1::shared_ptr<pstsdk::subnode_leaf_block> pstsdk::db_context_impl::read_subnode_leaf_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::subnode_leaf_block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_subnode_leaf_block(shared_from_this(), lookup_block_info(bid));
}

std::tr1::shared_ptr<pstsdk::subnode_nonleaf_block> pstsdk::db_context_impl::read_subnode_nonleaf_block(block_id bid)
{
    std::tr1::shared_ptr<pstsdk::subnode_nonleaf_block> presult;
    if(lookup_saved_block(bid, presult))
        return presult;
    return m_db->read_subnode_nonleaf_block(shared_from_this(), lookup_block_info(bid));
}

#endif
//! \endcond
