//! \file
//! \brief Heap-on-Node (HN) and BTree-on-Heap (BTH) implementation
//! \author Terry Mahaffey
//! \ingroup ltp

#ifndef PSTSDK_LTP_HEAP_H
#define PSTSDK_LTP_HEAP_H

#include <vector>
#include <algorithm>
#include <boost/noncopyable.hpp>
#include <boost/iostreams/concepts.hpp>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif
#include <boost/iostreams/stream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if __GNUC__
#include <tr1/memory>
#endif

#include "pstsdk/util/primitives.h"

#include "pstsdk/disk/disk.h"

#include "pstsdk/ndb/node.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4250)
#endif

namespace pstsdk
{

//! \defgroup ltp_heaprelated Heap
//! \ingroup ltp

template<typename K, typename V>
class bth_nonleaf_node;

template<typename K, typename V>
class bth_leaf_node;

template<typename K, typename V>
class bth_node;

class heap_impl;
typedef std::tr1::shared_ptr<heap_impl> heap_ptr;

//! \brief Defines a stream device for a heap allocation for use by boost iostream
//!
//! The boost iostream library requires that one defines a device, which
//! implements a few key operations. This is the device for streaming out
//! of a heap allocation.
//! \ingroup ltp_heaprelated
class hid_stream_device : public boost::iostreams::device<boost::iostreams::input_seekable>
{
public:
    hid_stream_device() : m_pos(0), m_hid(0) { }
    //! \copydoc node_stream_device::read()
    std::streamsize read(char* pbuffer, std::streamsize n);
    //! \copydoc node_stream_device::seek()
    std::streampos seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way);

private:
    friend class heap_impl;
    hid_stream_device(const heap_ptr& _heap, heap_id id) : m_pos(0), m_hid(id), m_pheap(_heap) { }

    std::streamsize m_pos;
    heap_id m_hid;
    heap_ptr m_pheap;
};

//! \brief The actual heap allocation stream, defined using the boost iostream library
//! and the \ref hid_stream_device.
//! \ingroup ltp_heaprelated
typedef boost::iostreams::stream<hid_stream_device> hid_stream;

//! \brief The HN implementation
//!
//! Similar to how a \ref node is implemented, the HN implemention is actually
//! divided up into two classes, heap_impl and heap, for the exact same
//! reasons. The implementation details are in heap_impl, which is always a 
//! dynamically allocated object, where as clients actually create heap
//! objects. As more and more "child" objects are created and opened from
//! inside the heap, they will reference the heap_impl as appropriate.
//! \ingroup ltp_heaprelated
class heap_impl : public std::tr1::enable_shared_from_this<heap_impl>
{
public:
    //! \brief Get the size of the given allocation
    //! \param[in] id The heap allocation to get the size of
    //! \throws length_error (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page or index of the allocation as indicated by the id are out of bounds for this node
    //! \returns The size of the allocation
    size_t size(heap_id id) const;
    
    //! \brief Get the count of pages (..external blocks) in this heap
    //! \returns The count of pages
    uint get_page_count() const { return m_node.get_page_count(); }
    
    //! \brief Returns the client root allocation out of this heap
    //!
    //! This value has specific meaning to the owner of the heap. It may point
    //! to a special structure which contains information about the data
    //! structures implemented in this heap or the larger node and subnodes
    //! (such as the \ref table). Or, it could just point to the root BTH 
    //! allocation (such as the \ref property_bag). In any event, the heap
    //! itself gives no special meaning to this value.
    //! \returns The client's root allocation
    heap_id get_root_id() const;
    
    //! \brief Returns the client signature of the heap
    //!
    //! Each heap is stamped with a signature byte by it's creator. This value
    //! is used mostly for validation purposes by the client when opening the
    //! heap.
    //! \sa heap_client_signature
    //! returns The client sig byte
    byte get_client_signature() const;
    
    //! \brief Read data out of a specified allocation at the specified offset
    //! \throws length_error (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page, index, or size of the allocation are out of bounds
    //! \param[in] buffer The location to store the data. The size of the buffer indicates the amount of data to read
    //! \param[in] id The heap allocation to read from
    //! \param[in] offset The offset into id to read starting at
    //! \returns The number of bytes read
    size_t read(std::vector<byte>& buffer, heap_id id, ulong offset) const;
    
    //! \brief Read an entire allocation
    //! \throws length_error (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If the page or index of the allocation as indicated by the id are out of bounds for this node
    //! \param[in] id The allocation to read
    //! \returns The entire allocation
    std::vector<byte> read(heap_id id) const;
    
    //! \brief Creates a stream device over a specified heap allocation
    //!
    //! The returned stream device can be used to construct a proper stream:
    //! \code
    //! heap h;
    //! hid_stream hstream(h.open_stream(0x30));
    //! \endcode
    //! Which can then be used as any iostream would be.
    //! \param[in] id The heap allocation to open a stream on
    //! \returns A heap allocation stream device
    hid_stream_device open_stream(heap_id id);
    //! \brief Get the node underlying this heap
    //! \returns The node
    const node& get_node() const { return m_node; }
    //! \brief Get the node underlying this heap
    //! \returns The node
    node& get_node() { return m_node; }

    //! \brief Opens a BTH from this heap with the given root id
    //! \tparam K The key type of this BTH
    //! \tparam V The value type of this BTH
    //! \param[in] root The root allocation of this BTH
    //! \returns The BTH object
    template<typename K, typename V>
    std::tr1::shared_ptr<bth_node<K,V> > open_bth(heap_id root);

    friend class heap;

private:
    heap_impl();
    explicit heap_impl(const node& n);
    heap_impl(const node& n, alias_tag);
    heap_impl(const node& n, byte client_sig);
    heap_impl(const node& n, byte client_sig, alias_tag);
    heap_impl(const heap_impl& other) 
        : m_node(other.m_node) { }

    node m_node;
};

//! \brief Heap-on-Node implementation
//!
//! The HN is the first concept built on top of the \ref node abstraction
//! exposed by the NDB. It treats the node similar to how a memory heap is
//! treated, allowing the client to allocate memory up to 3.8k and free 
//! those allocations from inside the node. To faciliate this, metadata is 
//! kept at the start and end of each block in the HN node (which are 
//! confusingly sometimes called pages in this context). So the HN has 
//! detailed knowledge of how blocks and extended_blocks work in order to
//! do it's book keeping, and find the appropriate block (..page) to 
//! satisfy a given allocation request.
//!
//! Note a heap completely controls a node - you can not have multiple
//! heaps per node. You can not use a node which has a heap on it for
//! any other purpose beyond the heap interface.
//! \sa [MS-PST 2.3.1]
//! \ingroup ltp_heaprelated
class heap
{
public:
    //! \brief Open a heap object on a node
    //! \param[in] n The node to open on top of. It will be copied.
    explicit heap(const node& n)
        : m_pheap(new heap_impl(n)) { }
    //! \brief Open a heap object on a node alias
    //! \param[in] n The node to alias
    heap(const node& n, alias_tag)
        : m_pheap(new heap_impl(n, alias_tag())) { }
    //! \brief Open a heap object on the specified node, and validate the client sig
    //! \throws sig_mismatch If the specified client_sig doesn't match what is in the node
    //! \param[in] n The node to open on top of. It will be copied.
    //! \param[in] client_sig Validate the heap has this value for the client sig
    heap(const node& n, byte client_sig)
        : m_pheap(new heap_impl(n, client_sig)) { }
    //! \brief Open a heap object on the specified node (alias), and validate the client sig
    //! \throws sig_mismatch If the specified client_sig doesn't match what is in the node
    //! \param[in] n The node to alias
    //! \param[in] client_sig Validate the heap has this value for the client sig
    heap(const node& n, byte client_sig, alias_tag)
        : m_pheap(new heap_impl(n, client_sig, alias_tag())) { }
    //! \brief Copy constructor
    //! \param[in] other The heap to copy
    heap(const heap& other)
        : m_pheap(new heap_impl(*(other.m_pheap))) { }
    //! \brief Alias constructor
    //! \param[in] other The heap to alias. The constructed object will share a heap_impl object with other.
    heap(const heap& other, alias_tag)
        : m_pheap(other.m_pheap) { }

#ifndef BOOST_NO_RVALUE_REFERENCES
    //! \brief Move constructor
    //! \param[in] other The heap to move from
    heap(heap&& other)
        : m_pheap(std::move(other.m_pheap)) { }
#endif

    //! \copydoc heap_impl::size()
    size_t size(heap_id id) const
        { return m_pheap->size(id); }
    //! \copydoc heap_impl::get_root_id()
    heap_id get_root_id() const
        { return m_pheap->get_root_id(); }
    //! \copydoc heap_impl::get_client_signature()
    byte get_client_signature() const
        { return m_pheap->get_client_signature(); }
    //! \copydoc heap_impl::read(std::vector<byte>&,heap_id,ulong) const
    size_t read(std::vector<byte>& buffer, heap_id id, ulong offset) const
        { return m_pheap->read(buffer, id, offset); }
    //! \copydoc heap_impl::read(heap_id) const
    std::vector<byte> read(heap_id id) const
        { return m_pheap->read(id); }
    //! \copydoc heap_impl::open_stream()
    hid_stream_device open_stream(heap_id id)
        { return m_pheap->open_stream(id); }

    //! \copydoc heap_impl::get_node() const
    const node& get_node() const
        { return m_pheap->get_node(); }
    //! \copydoc heap_impl::get_node()    
    node& get_node()
        { return m_pheap->get_node(); }
    
    //! \copydoc heap_impl::open_bth()
    template<typename K, typename V>
    std::tr1::shared_ptr<bth_node<K,V> > open_bth(heap_id root)
        { return m_pheap->open_bth<K,V>(root); }

private:
    heap& operator=(const heap& other); // = delete
    heap_ptr m_pheap;
};

//! \defgroup ltp_bthrelated BTH
//! \ingroup ltp

//! \brief The object which forms the root of the BTH hierarchy
//!
//! A BTH is a simple tree structure built using allocations out of a 
//! \ref heap. bth_node forms the root of the object hierarchy 
//! representing this tree. The child classes bth_nonleaf_node and 
//! bth_leaf_node contain the implementation and represent nonleaf and
//! leaf bth_nodes, respectively.
//!
//! Because a single bth_node is backed by a HN allocation (max 3.8k),
//! most BTH "trees" consist of a BTH header which points directly to a
//! single BTH leaf node.
//!
//! This hierarchy also models the \ref btree_node structure, inheriting the 
//! actual iteration and lookup logic.
//! \tparam K The key type for this BTH
//! \tparam V The value type for this BTH
//! \ingroup ltp_bthrelated
//! \sa [MS-PST] 2.3.2
template<typename K, typename V>
class bth_node : 
    public virtual btree_node<K,V>, 
    private boost::noncopyable
{
public:
    //! \brief Opens a BTH node from the specified heap at the given root
    //! \throws sig_mismatch (\ref PSTSDK_VALIDATION_LEVEL_WEAK) If this allocation doesn't have the BTH stamp
    //! \throws logic_error If the specified key/value type sizes do not match what is in the BTH header
    //! \param[in] h The heap to open out of
    //! \param[in] bth_root The allocation containing the bth header
    static std::tr1::shared_ptr<bth_node<K,V> > open_root(const heap_ptr& h, heap_id bth_root);
    //! \brief Open a non-leaf BTH node
    //! \param[in] h The heap to open out of
    //! \param[in] id The id to interpret as a non-leaf BTH node
    //! \param[in] level The level of this non-leaf node (must be non-zero)
    static std::tr1::shared_ptr<bth_nonleaf_node<K,V> > open_nonleaf(const heap_ptr& h, heap_id id, ushort level);
    //! \brief Open a leaf BTH node
    //! \param[in] h The heap to open out of
    //! \param[in] id The id to interpret as a leaf BTH node   
    static std::tr1::shared_ptr<bth_leaf_node<K,V> > open_leaf(const heap_ptr& h, heap_id id);

    //! \brief Construct a bth_node object
    //! \param[in] h The heap to open out of
    //! \param[in] id The id to interpret as a non-leaf BTH node
    //! \param[in] level The level of this non-leaf node (must be non-zero)
    bth_node(const heap_ptr& h, heap_id id, ushort level)
        : m_heap(h), m_id(id), m_level(level) { }
    virtual ~bth_node() { }

    //! \brief Return the heap_id of this bth_node
    //! \returns the heap_id of this bth_node
    heap_id get_id() const { return m_id; }
    //! \brief Return the leve of this bth_node
    //! \returns the leve of this bth_node
    ushort get_level() const { return m_level; }
    //! \brief Return the key size of this bth
    //! \returns the key size of this bth
    size_t get_key_size() const { return sizeof(K); }
    //! \brief Return the value size of this bth
    //! \returns the value size of this bth
    size_t get_value_size() const { return sizeof(V); }

    //! \brief Returns the heap this bth_node is in
    //! \brief The heap this bth_node is in
    const heap_ptr get_heap_ptr() const { return m_heap; }
    //! \brief Returns the heap this bth_node is in
    //! \brief The heap this bth_node is in
    heap_ptr get_heap_ptr() { return m_heap; }

    //! \brief Get the node underlying this BTH
    //! \returns the node
    const node& get_node() const { return m_heap->get_node(); }
    //! \brief Get the node underlying this BTH
    //! \returns the node
    node& get_node() { return m_heap->get_node(); }

protected:
    heap_ptr m_heap;

private:
    heap_id m_id;   //!< The heap_id of this bth_node
    ushort m_level; //!< The level of this bth_node (0 for leaf, or distance from leaf)
};

//! \brief Contains references to other bth_node allocations
//! \tparam K The key type for this BTH
//! \tparam V The value type for this BTH
//! \ingroup ltp_bthrelated
//! \sa [MS-PST] 2.3.2.2
template<typename K, typename V>
class bth_nonleaf_node : 
    public bth_node<K,V>, 
    public btree_node_nonleaf<K,V>
{
public:
    //! \brief Construct a bth_nonleaf_node
    //! \param[in] h The heap to open out of
    //! \param[in] id The id to interpret as a non-leaf BTH node
    //! \param[in] level The level of this bth_nonleaf_node (non-zero)
    //! \param[in] bth_info The info about child bth_node allocations
#ifndef BOOST_NO_RVALUE_REFERENCES
    bth_nonleaf_node(const heap_ptr& h, heap_id id, ushort level, std::vector<std::pair<K, heap_id> > bth_info)
        : bth_node<K,V>(h, id, level), m_bth_info(std::move(bth_info)), m_child_nodes(m_bth_info.size()) { }
#else
    bth_nonleaf_node(const heap_ptr& h, heap_id id, ushort level, const std::vector<std::pair<K, heap_id> >& bth_info)
        : bth_node<K,V>(h, id, level), m_bth_info(bth_info), m_child_nodes(m_bth_info.size()) { }
#endif
    // btree_node_nonleaf implementation
    const K& get_key(uint pos) const { return m_bth_info[pos].first; }
    bth_node<K,V>* get_child(uint pos);
    const bth_node<K,V>* get_child(uint pos) const;
    uint num_values() const { return m_child_nodes.size(); }

private:
    std::vector<std::pair<K, heap_id> > m_bth_info;
    mutable std::vector<std::tr1::shared_ptr<bth_node<K,V> > > m_child_nodes;
};

//! \brief Contains the actual key value pairs of the BTH
//! \tparam K The key type for this BTH
//! \tparam V The value type for this BTH
//! \ingroup ltp_bthrelated
//! \sa [MS-PST] 2.3.2.3
template<typename K, typename V>
class bth_leaf_node : 
    public bth_node<K,V>, 
    public btree_node_leaf<K,V>
{
public:
    //! \brief Construct a bth_leaf_node
    //! \param[in] h The heap to open out of
    //! \param[in] id The id to interpret as a non-leaf BTH node
    //! \param[in] data The key/value pairs stored in this leaf
#ifndef BOOST_NO_RVALUE_REFERENCES
    bth_leaf_node(const heap_ptr& h, heap_id id, std::vector<std::pair<K,V> > data)
        : bth_node<K,V>(h, id, 0), m_bth_data(std::move(data)) { }
#else
    bth_leaf_node(const heap_ptr& h, heap_id id, const std::vector<std::pair<K,V> >& data)
        : bth_node<K,V>(h, id, 0), m_bth_data(data) { }
#endif

    virtual ~bth_leaf_node() { }

    // btree_node_leaf implementation
    const V& get_value(uint pos) const
        { return m_bth_data[pos].second; }
    const K& get_key(uint pos) const
        { return m_bth_data[pos].first; }
    uint num_values() const
        { return m_bth_data.size(); }

private:
    std::vector<std::pair<K,V> > m_bth_data;
};

} // end pstsdk namespace

template<typename K, typename V>
inline std::tr1::shared_ptr<pstsdk::bth_node<K,V> > pstsdk::bth_node<K,V>::open_root(const heap_ptr& h, heap_id bth_root)
{
    disk::bth_header* pheader;
    std::vector<byte> buffer(sizeof(disk::bth_header));
    pheader = (disk::bth_header*)&buffer[0];

    h->read(buffer, bth_root, 0);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(pheader->bth_signature != disk::heap_sig_bth)
        throw sig_mismatch("bth_signature expected", 0, bth_root, pheader->bth_signature, disk::heap_sig_bth);

    if(pheader->key_size != sizeof(K))
        throw std::logic_error("invalid key size");

    if(pheader->entry_size != sizeof(V))
        throw std::logic_error("invalid entry size");
#endif

    if(pheader->num_levels > 1)
        return open_nonleaf(h, pheader->root, pheader->num_levels-1);
    else
        return open_leaf(h, pheader->root);
}

template<typename K, typename V>
inline std::tr1::shared_ptr<pstsdk::bth_nonleaf_node<K,V> > pstsdk::bth_node<K,V>::open_nonleaf(const heap_ptr& h, heap_id id, ushort level)
{
    uint num_entries = h->size(id) / sizeof(disk::bth_nonleaf_entry<K>);
    std::vector<byte> buffer(h->size(id));
    disk::bth_nonleaf_node<K>* pbth_nonleaf_node = (disk::bth_nonleaf_node<K>*)&buffer[0];
    std::vector<std::pair<K, heap_id> > child_nodes;

    h->read(buffer, id, 0);

    child_nodes.reserve(num_entries);

    for(uint i = 0; i < num_entries; ++i)
    {
        child_nodes.push_back(std::make_pair(pbth_nonleaf_node->entries[i].key, pbth_nonleaf_node->entries[i].page));
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    return std::tr1::shared_ptr<bth_nonleaf_node<K,V> >(new bth_nonleaf_node<K,V>(h, id, level, std::move(child_nodes)));
#else
    return std::tr1::shared_ptr<bth_nonleaf_node<K,V> >(new bth_nonleaf_node<K,V>(h, id, level, child_nodes));
#endif
}
    
template<typename K, typename V>
inline std::tr1::shared_ptr<pstsdk::bth_leaf_node<K,V> > pstsdk::bth_node<K,V>::open_leaf(const heap_ptr& h, heap_id id)
{
    std::vector<std::pair<K, V> > entries; 

    if(id)
    {
        uint num_entries = h->size(id) / sizeof(disk::bth_leaf_entry<K,V>);
        std::vector<byte> buffer(h->size(id));
        disk::bth_leaf_node<K,V>* pbth_leaf_node = (disk::bth_leaf_node<K,V>*)&buffer[0];

        h->read(buffer, id, 0);

        entries.reserve(num_entries);

        for(uint i = 0; i < num_entries; ++i)
        {
            entries.push_back(std::make_pair(pbth_leaf_node->entries[i].key, pbth_leaf_node->entries[i].value));
        }
#ifndef BOOST_NO_RVALUE_REFERENCES
        return std::tr1::shared_ptr<bth_leaf_node<K,V> >(new bth_leaf_node<K,V>(h, id, std::move(entries)));
#else
        return std::tr1::shared_ptr<bth_leaf_node<K,V> >(new bth_leaf_node<K,V>(h, id, entries));
#endif
    }
    else
    {
        // id == 0 means an empty tree
        return std::tr1::shared_ptr<bth_leaf_node<K,V> >(new bth_leaf_node<K,V>(h, id, entries));
    }
}

template<typename K, typename V>
inline pstsdk::bth_node<K,V>* pstsdk::bth_nonleaf_node<K,V>::get_child(uint pos)
{
    if(m_child_nodes[pos] == NULL)
    {
        if(this->get_level() > 1)
            m_child_nodes[pos] = bth_node<K,V>::open_nonleaf(this->get_heap_ptr(), m_bth_info[pos].second, this->get_level()-1);
        else
            m_child_nodes[pos] = bth_node<K,V>::open_leaf(this->get_heap_ptr(), m_bth_info[pos].second);
    }

    return m_child_nodes[pos].get();
}

template<typename K, typename V>
inline const pstsdk::bth_node<K,V>* pstsdk::bth_nonleaf_node<K,V>::get_child(uint pos) const
{
    if(m_child_nodes[pos] == NULL)
    {
        if(this->get_level() > 1)
            m_child_nodes[pos] = bth_node<K,V>::open_nonleaf(this->get_heap_ptr(), m_bth_info[pos].second, this->get_level()-1);
        else
            m_child_nodes[pos] = bth_node<K,V>::open_leaf(this->get_heap_ptr(), m_bth_info[pos].second);
    }

    return m_child_nodes[pos].get();
}

inline pstsdk::heap_impl::heap_impl(const node& n)
: m_node(n)
{
    // need to throw if the node is smaller than first_header
    disk::heap_first_header first_header = m_node.read<disk::heap_first_header>(0);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(first_header.signature != disk::heap_signature)
        throw sig_mismatch("invalid heap_sig", 0, n.get_id(), first_header.signature, disk::heap_signature);
#endif
}

inline pstsdk::heap_impl::heap_impl(const node& n, alias_tag)
: m_node(n, alias_tag())
{
    // need to throw if the node is smaller than first_header
    disk::heap_first_header first_header = m_node.read<disk::heap_first_header>(0);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(first_header.signature != disk::heap_signature)
        throw sig_mismatch("invalid heap_sig", 0, n.get_id(), first_header.signature, disk::heap_signature);
#endif
}

inline pstsdk::heap_impl::heap_impl(const node& n, byte client_sig)
: m_node(n)
{
    // need to throw if the node is smaller than first_header
    disk::heap_first_header first_header = m_node.read<disk::heap_first_header>(0);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(first_header.signature != disk::heap_signature)
        throw sig_mismatch("invalid heap_sig", 0, n.get_id(), first_header.signature, disk::heap_signature);
#endif
    if(first_header.client_signature != client_sig)
        throw sig_mismatch("invalid client_sig", 0, n.get_id(), first_header.client_signature, client_sig);
}

inline pstsdk::heap_impl::heap_impl(const node& n, byte client_sig, alias_tag)
: m_node(n, alias_tag())
{
    // need to throw if the node is smaller than first_header
    disk::heap_first_header first_header = m_node.read<disk::heap_first_header>(0);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(first_header.signature != disk::heap_signature)
        throw sig_mismatch("invalid heap_sig", 0, n.get_id(), first_header.signature, disk::heap_signature);
#endif
    if(first_header.client_signature != client_sig)
        throw sig_mismatch("invalid client_sig", 0, n.get_id(), first_header.client_signature, client_sig);
}

inline pstsdk::heap_id pstsdk::heap_impl::get_root_id() const
{
    disk::heap_first_header first_header = m_node.read<disk::heap_first_header>(0);
    return first_header.root_id;
}

inline pstsdk::byte pstsdk::heap_impl::get_client_signature() const
{
    disk::heap_first_header first_header = m_node.read<disk::heap_first_header>(0);
    return first_header.client_signature;
}

inline size_t pstsdk::heap_impl::size(heap_id id) const
{
    if(id == 0)
        return 0;

    disk::heap_page_header header = m_node.read<disk::heap_page_header>(get_heap_page(id), 0);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(header.page_map_offset > m_node.get_page_size(get_heap_page(id)))
        throw std::length_error("page_map_offset > node size");
#endif

    std::vector<byte> buffer(m_node.get_page_size(get_heap_page(id)) - header.page_map_offset);
    m_node.read(buffer, get_heap_page(id), header.page_map_offset);
    disk::heap_page_map* pmap = reinterpret_cast<disk::heap_page_map*>(&buffer[0]);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(get_heap_index(id) > pmap->num_allocs)
        throw std::length_error("index > num_allocs");
#endif

    return pmap->allocs[get_heap_index(id) + 1] - pmap->allocs[get_heap_index(id)];
}

inline size_t pstsdk::heap_impl::read(std::vector<byte>& buffer, heap_id id, ulong offset) const
{
    size_t hid_size = size(id);

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(buffer.size() > hid_size)
        throw std::length_error("buffer.size() > size()");

    if(offset > hid_size)
        throw std::length_error("offset > size()");

    if(offset + buffer.size() > hid_size)
        throw std::length_error("size + offset > size()");
#endif

    if(hid_size == 0)
        return 0;

    disk::heap_page_header header = m_node.read<disk::heap_page_header>(get_heap_page(id), 0);
    std::vector<byte> map_buffer(m_node.get_page_size(get_heap_page(id)) - header.page_map_offset);
    m_node.read(map_buffer, get_heap_page(id), header.page_map_offset);
    disk::heap_page_map* pmap = reinterpret_cast<disk::heap_page_map*>(&map_buffer[0]);

    return m_node.read(buffer, get_heap_page(id), pmap->allocs[get_heap_index(id)] + offset);
}

inline pstsdk::hid_stream_device pstsdk::heap_impl::open_stream(heap_id id)
{
    return hid_stream_device(shared_from_this(), id);
}

inline std::streamsize pstsdk::hid_stream_device::read(char* pbuffer, std::streamsize n)
{
    if(m_hid && (static_cast<size_t>(m_pos) + n > m_pheap->size(m_hid)))
        n = m_pheap->size(m_hid) - m_pos;

    if(n == 0 || m_hid == 0)
        return -1;

    std::vector<byte> buff(static_cast<uint>(n));
    size_t read = m_pheap->read(buff, m_hid, static_cast<ulong>(m_pos));

    memcpy(pbuffer, &buff[0], read);

    m_pos += read;

    return read;
}

inline std::streampos pstsdk::hid_stream_device::seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way)
{
#if defined(_MSC_VER) && (_MSC_VER < 1600)
#pragma warning(push)
#pragma warning(disable:4244)
#endif
    if(way == std::ios_base::beg)
        m_pos = off;
    else if(way == std::ios_base::end)
        m_pos = m_pheap->size(m_hid) + off;
    else
        m_pos += off;
#if defined(_MSC_VER) && (_MSC_VER < 1600)
#pragma warning(pop)
#endif

    if(m_pos < 0)
        m_pos = 0;
    else if(static_cast<size_t>(m_pos) > m_pheap->size(m_hid))
        m_pos = m_pheap->size(m_hid);

    return m_pos;
}

inline std::vector<pstsdk::byte> pstsdk::heap_impl::read(heap_id id) const
{
    std::vector<byte> result(size(id));
    read(result, id, 0);
    return result;
}

template<typename K, typename V>
inline std::tr1::shared_ptr<pstsdk::bth_node<K,V> > pstsdk::heap_impl::open_bth(heap_id root)
{ 
    return bth_node<K,V>::open_root(shared_from_this(), root); 
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
