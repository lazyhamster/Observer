//! \file
//! \brief Property Bag (or Property Context, or PC) implementation
//! \author Terry Mahaffey
//! \ingroup ltp

#ifndef PSTSDK_LTP_PROPBAG_H
#define PSTSDK_LTP_PROPBAG_H

#include <vector>
#include <algorithm>

#include "pstsdk/util/primitives.h"
#include "pstsdk/util/errors.h"

#include "pstsdk/ndb/node.h"

#include "pstsdk/ltp/object.h"
#include "pstsdk/ltp/heap.h"

namespace pstsdk
{

//! \addtogroup ltp_objectrelated
//@{
typedef bth_node<prop_id, disk::prop_entry> pc_bth_node;
typedef bth_nonleaf_node<prop_id, disk::prop_entry> pc_bth_nonleaf_node;
typedef bth_leaf_node<prop_id, disk::prop_entry> pc_bth_leaf_node;
//@}

//! \brief Property Context (PC) Implementation
//!
//! A Property Context is simply a BTH where the BTH is stored as the client
//! root allocation in the heap. The BTH contains a "prop_entry", which is
//! defines the type of the property and it's storage. 
//!
//! const_property_object does most of the heavy lifting in terms of 
//! property access and interpretation.
//! \sa [MS-PST] 2.3.3
//! \ingroup ltp_objectrelated
class property_bag : public const_property_object
{
public:
    //! \brief Construct a property_bag from this node
    //! \param[in] n The node to copy and interpret as a property_bag
    explicit property_bag(const node& n);
    //! \brief Construct a property_bag from this node
    //! \param[in] n The node to alias and interpret as a property_bag
    property_bag(const node& n, alias_tag);
    //! \brief Construct a property_bag from this heap
    //! \param[in] h The heap to copy and interpret as a property_bag
    explicit property_bag(const heap& h);
    //! \brief Construct a property_bag from this heap
    //! \param[in] h The heap to alias and interpret as a property_bag
    property_bag(const heap& h, alias_tag);
    //! \brief Copy construct a property_bag
    //! \param other The property bag to copy
    property_bag(const property_bag& other);
    //! \brief Alias a property_bag
    //! \param other The property bag to alias
    property_bag(const property_bag& other, alias_tag);

#ifndef BOOST_NO_RVALUE_REFERENCES
    //! \brief Move construct a property_bag
    //! \param other The property bag to move from
    property_bag(property_bag&& other) : m_pbth(std::move(other.m_pbth)) { }
#endif

    std::vector<prop_id> get_prop_list() const;
    prop_type get_prop_type(prop_id id) const
        { return (prop_type)m_pbth->lookup(id).type; }
    bool prop_exists(prop_id id) const;
    size_t size(prop_id id) const;
    hnid_stream_device open_prop_stream(prop_id id);
    
    //! \brief Get the node underlying this property_bag
    //! \returns The node
    const node& get_node() const { return m_pbth->get_node(); }
    //! \brief Get the node underlying this property_bag
    //! \returns The node
    node& get_node() { return m_pbth->get_node(); }

private:
    property_bag& operator=(const property_bag& other); // = delete

    byte get_value_1(prop_id id) const
        { return (byte)m_pbth->lookup(id).id; }
    ushort get_value_2(prop_id id) const
        { return (ushort)m_pbth->lookup(id).id; }
    ulong get_value_4(prop_id id) const
        { return (ulong)m_pbth->lookup(id).id; }
    ulonglong get_value_8(prop_id id) const;
    std::vector<byte> get_value_variable(prop_id id) const;
    void get_prop_list_impl(std::vector<prop_id>& proplist, const pc_bth_node* pbth_node) const;

    std::tr1::shared_ptr<pc_bth_node> m_pbth;
};

} // end pstsdk namespace

inline pstsdk::property_bag::property_bag(const pstsdk::node& n)
{
    heap h(n, disk::heap_sig_pc);

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline pstsdk::property_bag::property_bag(const pstsdk::node& n, alias_tag)
{
    heap h(n, disk::heap_sig_pc, alias_tag());

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline pstsdk::property_bag::property_bag(const pstsdk::heap& h)
{
#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(h.get_client_signature() != disk::heap_sig_pc)
        throw sig_mismatch("expected heap_sig_pc", 0, h.get_node().get_id(), h.get_client_signature(), disk::heap_sig_pc);
#endif

    heap my_heap(h);

    m_pbth = my_heap.open_bth<prop_id, disk::prop_entry>(my_heap.get_root_id());
}

inline pstsdk::property_bag::property_bag(const pstsdk::heap& h, alias_tag)
{
#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(h.get_client_signature() != disk::heap_sig_pc)
        throw sig_mismatch("expected heap_sig_pc", 0, h.get_node().get_id(), h.get_client_signature(), disk::heap_sig_pc);
#endif

    heap my_heap(h, alias_tag());

    m_pbth = my_heap.open_bth<prop_id, disk::prop_entry>(my_heap.get_root_id());
}

inline pstsdk::property_bag::property_bag(const property_bag& other)
{
    heap h(other.m_pbth->get_node());

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline pstsdk::property_bag::property_bag(const property_bag& other, alias_tag)
{
    heap h(other.m_pbth->get_node(), alias_tag());

    m_pbth = h.open_bth<prop_id, disk::prop_entry>(h.get_root_id());
}

inline std::vector<pstsdk::prop_id> pstsdk::property_bag::get_prop_list() const
{
    std::vector<prop_id> proplist;

    get_prop_list_impl(proplist, m_pbth.get());

    return proplist;
}

inline void pstsdk::property_bag::get_prop_list_impl(std::vector<prop_id>& proplist, const pc_bth_node* pbth_node) const
{
    if(pbth_node->get_level() == 0)
    {
        // leaf
        const pc_bth_leaf_node* pleaf = static_cast<const pc_bth_leaf_node*>(pbth_node);

        for(uint i = 0; i < pleaf->num_values(); ++i)
            proplist.push_back(pleaf->get_key(i));
    }
    else
    {
        // non-leaf
        const pc_bth_nonleaf_node* pnonleaf = static_cast<const pc_bth_nonleaf_node*>(pbth_node); 
        for(uint i = 0; i < pnonleaf->num_values(); ++i)
            get_prop_list_impl(proplist, pnonleaf->get_child(i));
    }
}

inline bool pstsdk::property_bag::prop_exists(prop_id id) const
{
    try
    {
        (void)m_pbth->lookup(id);
    }
    catch(key_not_found<prop_id>&)
    {
        return false;
    }

    return true;
}


inline pstsdk::ulonglong pstsdk::property_bag::get_value_8(prop_id id) const
{
    std::vector<byte> buffer = get_value_variable(id);

    return *(ulonglong*)&buffer[0];
}

inline std::vector<pstsdk::byte> pstsdk::property_bag::get_value_variable(prop_id id) const
{
    heapnode_id h_id = (heapnode_id)get_value_4(id);
    std::vector<byte> buffer;

    if(is_subnode_id(h_id))
    {
        node sub(m_pbth->get_node().lookup(h_id));
        buffer.resize(sub.size());
        sub.read(buffer, 0);
    }
    else
    {
        buffer = m_pbth->get_heap_ptr()->read(h_id);
    }

    return buffer;
}


inline size_t pstsdk::property_bag::size(prop_id id) const
{
    heapnode_id h_id = (heapnode_id)get_value_4(id);

    if(is_subnode_id(h_id))
        return node(m_pbth->get_node().lookup(h_id)).size();
    else
        return m_pbth->get_heap_ptr()->size(h_id);
}

inline pstsdk::hnid_stream_device pstsdk::property_bag::open_prop_stream(prop_id id)
{
    heapnode_id h_id = (heapnode_id)get_value_4(id);

    if(h_id == 0)
        return m_pbth->get_heap_ptr()->open_stream(h_id);

    if(is_subnode_id(h_id))
        return m_pbth->get_node().lookup(h_id).open_as_stream();
    else
        return m_pbth->get_heap_ptr()->open_stream(h_id);
}
#endif
