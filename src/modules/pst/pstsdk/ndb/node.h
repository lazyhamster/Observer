//! \file
//! \brief Node and Block definitions
//! \author Terry Mahaffey
//!
//! The concept of a node is the primary abstraction exposed by the NDB layer.
//! Closely related is the concept of blocks, also defined here.
//! \ingroup ndb

#ifndef PSTSDK_NDB_NODE_H
#define PSTSDK_NDB_NODE_H

#include <vector>
#include <algorithm>
#include <memory>
#include <cassert>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iostreams/concepts.hpp>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif
#include <boost/iostreams/stream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "pstsdk/util/util.h"
#include "pstsdk/util/btree.h"

#include "pstsdk/ndb/database_iface.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4250)
#endif

namespace pstsdk
{

//! \defgroup ndb_noderelated Node
//! \ingroup ndb

//! \brief The node implementation
//!
//! The node class is really divided into two classes, node and
//! node_impl. The purpose of this design is to allow a node to conceptually
//! "outlive" the stack based object encapsulating it. This is necessary
//! if subobjects are opened which still need to refer to their parent.
//! This is accomplished in this case, like others, via having a thin stack
//! based object (\ref node) with a shared pointer to the implemenetation object
//! (node_impl). Subnodes will grab a shared_ptr ref to the parents node_impl.
//!
//! See the documentation for \ref node to read more about what a node is.
//! 
//! The interface of node_impl follows closely that of node; differing mainly 
//! in the construction and destruction semantics plus the addition of some 
//! convenience functions.
//! 
//! \ref node and its \ref node_impl class generally have a one to one mapping,
//! this isn't true only if someone opens an \ref alias_tag "alias" for a node.
//! \ingroup ndb_noderelated
class node_impl : public std::tr1::enable_shared_from_this<node_impl>
{
public:
    //! \brief Constructor for top level nodes
    //!
    //! This constructor is specific to nodes defined in the NBT
    //! \param[in] db The database context we're located in
    //! \param[in] info Information about this node
    node_impl(const shared_db_ptr& db, const node_info& info)
        : m_id(info.id), m_original_data_id(info.data_bid), m_original_sub_id(info.sub_bid), m_original_parent_id(info.parent_id), m_parent_id(info.parent_id), m_db(db) { }

    //! \brief Constructor for subnodes
    //!
    //! This constructor is specific to nodes defined in other nodes
    //! \param[in] container_node The parent or containing node
    //! \param[in] info Information about this node
    node_impl(const std::tr1::shared_ptr<node_impl>& container_node, const subnode_info& info)
        : m_id(info.id), m_original_data_id(info.data_bid), m_original_sub_id(info.sub_bid), m_original_parent_id(0), m_parent_id(0), m_pcontainer_node(container_node), m_db(container_node->m_db) { }

    //! \brief Set one node equal to another
    //!
    //! The assignment semantics of a node cause the assigned to node to refer
    //! to the same data on disk as the assigned from node. It still has it's 
    //! own unique id, parent, etc - only the data contained in this node is 
    //! 'assigned'
    //! \param[in] other The node to assign from
    //! \returns *this after the assignment is done
    node_impl& operator=(const node_impl& other)
        { m_pdata = other.m_pdata; m_psub = other.m_psub; return *this; }

    //! \brief Get the id of this node
    //! \returns The id
    node_id get_id() const { return m_id; }
    //! \brief Get the block_id of the data block of this node
    //! \returns The id
    block_id get_data_id() const;
    //! \brief Get the block_id of the subnode block of this node
    //! \returns The id
    block_id get_sub_id() const;

    //! \brief Get the parent id
    //!
    //! The parent id here is the field in the NBT. It is not the id of the
    //! container node, if any. Generally the parent id of a message will
    //! be a folder, etc.
    //! \returns The id, zero if this is a subnode
    node_id get_parent_id() const { return m_parent_id; }

    //! \brief Tells you if this is a subnode
    //! \returns true if this is a subnode, false otherwise
    bool is_subnode() { return m_pcontainer_node; }

    //! \brief Returns the data block associated with this node
    //! \returns A shared pointer to the data block
    std::tr1::shared_ptr<data_block> get_data_block() const
        { ensure_data_block(); return m_pdata; }
    //! \brief Returns the subnode block associated with this node
    //! \returns A shared pointer to the subnode block
    std::tr1::shared_ptr<subnode_block> get_subnode_block() const 
        { ensure_sub_block(); return m_psub; }
    
    //! \brief Read data from this node
    //!
    //! Fills the specified buffer with data starting at the specified offset.
    //! The size of the buffer indicates how much data to read.
    //! \param[in,out] buffer The buffer to fill
    //! \param[in] offset The location to read from
    //! \returns The amount of data read
    size_t read(std::vector<byte>& buffer, ulong offset) const;
    
    //! \brief Read data from this node
    //!
    //! Returns a "T" located as the specified offset
    //! \tparam T The type to read
    //! \param[in] offset The location to read from
    //! \returns The type read
    template<typename T> T read(ulong offset) const;

    //! \brief Read data from this node
    //!
    //! Fills the specified buffer with data on the specified page at the
    //! specified offset. The size of teh buffer indicates how much data to
    //! read.
    //! \note In this context, a "page" is an external block
    //! \param[in,out] buffer The buffer to fill
    //! \param[in] page_num The page to read from
    //! \param[in] offset The location to read from
    //! \returns The amount of data read
    size_t read(std::vector<byte>& buffer, uint page_num, ulong offset) const;
    
    //! \brief Read data from a specific block on this node
    //! \note In this context, a "page" is an external block
    //! \tparam T The type to read
    //! \param[in] page_num The block (ordinal) to read data from
    //! \param[in] offset The offset into that block to read from
    //! \returns The type read
    template<typename T> T read(uint page_num, ulong offset) const;

    //! \brief Read data from this node
    //! \param[out] pdest_buffer The location to read the data into
    //! \param[in] size The amount of data to read
    //! \param[in] offset The location to read from
    //! \returns The amount of data read
    size_t read_raw(byte* pdest_buffer, size_t size, ulong offset) const;

//! \cond write_api
    size_t write(const std::vector<byte>& buffer, ulong offset);
    template<typename T> void write(const T& obj, ulong offset);
    size_t write(const std::vector<byte>& buffer, uint page_num, ulong offset);
    template<typename T> void write(const T& obj, uint page_num, ulong offset);
    size_t write_raw(const byte* pdest_buffer, size_t size, ulong offset);
//! \endcond

    //! \brief Returns the size of this node
    //! \returns The node size
    size_t size() const;
//! \cond write_api
    size_t resize(size_t size);
//! \endcond

    //! \brief Returns the size of a page in this node
    //! \note In this context, a "page" is an external block
    //! \param[in] page_num The page to get the size of
    //! \returns The size of the page
    size_t get_page_size(uint page_num) const;

    //! \brief Returns the number of pages in this node
    //! \note In this context, a "page" is an external block
    //! \returns The number of pages
    uint get_page_count() const;

    // iterate over subnodes
    //! \brief Returns an iterator positioned at first subnodeinfo
    //! \returns An iterator over subnodeinfos
    const_subnodeinfo_iterator subnode_info_begin() const;
    //! \brief Returns an iterator positioned past the last subnodeinfo
    //! \returns An iterator over subnodeinfos
    const_subnodeinfo_iterator subnode_info_end() const;

    //! \brief Lookup a subnode by node id
    //! \throws key_not_found<node_id> if a subnode with the specified node_id was not found
    //! \param[in] id The subnode id to find
    //! \returns The subnode
    node lookup(node_id id) const;

private:
    //! \brief Loads the data block from disk
    //! \returns The data block for this node
    data_block* ensure_data_block() const;
    //! \brief Loads the subnode block from disk
    //! \returns The subnode block for this node
    subnode_block* ensure_sub_block() const;

    const node_id m_id;             //!< The node_id of this node
    block_id m_original_data_id;    //!< The original block_id of the data block of this node
    block_id m_original_sub_id;     //!< The original block_id of the subnode block of this node
    node_id m_original_parent_id;   //!< The original node_id of the parent node of this node

    mutable std::tr1::shared_ptr<data_block> m_pdata;    //!< The data block
    mutable std::tr1::shared_ptr<subnode_block> m_psub;  //!< The subnode block
    node_id m_parent_id;                            //!< The parent node_id to this node

    std::tr1::shared_ptr<node_impl> m_pcontainer_node;   //!< The container node, of which we're a subnode, if applicable

    shared_db_ptr m_db; //!< The database context pointer
};

//! \brief Defines a transform from a subnode_info to an actual node object
//! 
//! Nodes <i>actually</i> contain a collection of subnode_info objects, not
//! a collection of subnodes. In order to provide the friendly facade of nodes
//! appearing to have a collection of subnodes, we define this transform. It is
//! used later with the boost iterator library to define a proxy iterator
//! for the subnodes of a node.
//! \ingroup ndb_noderelated
class subnode_transform_info : public std::unary_function<subnode_info, node>
{
public:
    //! \brief Initialize this functor with the container node involved
    //! \param[in] parent The containing node
    subnode_transform_info(const std::tr1::shared_ptr<node_impl>& parent)
        : m_parent(parent) { }

    //! \brief Given a subnode_info, construct a subnode
    //! \param[in] info The info about the subnode
    //! \returns A constructed node
    node operator()(const subnode_info& info) const;

private:
    std::tr1::shared_ptr<node_impl> m_parent; //!< The container node
};

//! \brief Defines a stream device for a node for use by boost iostream
//!
//! The boost iostream library requires that one defines a device, which
//! implements a few key operations. This is the device for streaming out
//! of a node.
//! \ingroup ndb_noderelated
class node_stream_device : public boost::iostreams::device<boost::iostreams::seekable>
{
public:
    //! \brief Default construct the node stream device
    node_stream_device() : m_pos(0) { }

    //! \brief Read data from this node into the buffer at the current position
    //! \param[out] pbuffer The buffer to store the results into
    //! \param[in] n The amount of data to read
    //! \returns The amount of data read
    std::streamsize read(char* pbuffer, std::streamsize n); 

    //! \cond write_api
    std::streamsize write(const char* pbuffer, std::streamsize n);
    //! \endcond

    //! \brief Move the current position in the stream
    //! \param[in] off The offset to move the current position
    //! \param[in] way The location to move the current position from
    //! \returns The new position
    std::streampos seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way);

private:
    friend class node;
    //! \brief Construct the device from a node
    node_stream_device(std::tr1::shared_ptr<node_impl>& _node) : m_pos(0), m_pnode(_node) { }

    std::streamsize m_pos;              //!< The stream's current position
    std::tr1::shared_ptr<node_impl> m_pnode; //!< The node this stream is over
};

//! \brief The actual node stream, defined using the boost iostream library
//! and the \ref node_stream_device.
//! \ingroup ndb_noderelated
typedef boost::iostreams::stream<node_stream_device> node_stream;

//! \brief An in memory representation of the "node" concept in a PST data file
//!
//! A node is the primary abstraction exposed by the \ref ndb to the
//! upper layers. It's purpose is to abstract away the details of working with
//! immutable blocks and subnode blocks. It can therefor be thought of simply
//! as a mutable stream of bytes and a collection of sub nodes, themselves
//! a mutable stream of bytes potentially with another collection of subnodes,
//! etc.
//!
//! When using the \ref node class, think of it as creating an in memory 
//! "instance" of the node on the disk. You can have several in memory
//! instances of the same node on disk. You can even have an \ref alias_tag 
//! "alias" of another in memory instance, as is sometimes required when 
//! creating higher level abstractions. 
//!
//! There isn't much interesting to do with a node, you can query its size,
//! read from a specific location, get it's id and parent id, iterate over
//! subnodes, etc. Most of the interesting things are done by higher level
//! abstractions which are built on top of and know specifically how to 
//! interpret the bytes in a node - such as the \ref heap, \ref table, and \ref
//! property_bag.
//! \sa [MS-PST] 2.2.1.1
//! \ingroup ndb_noderelated
class node
{
public:
    //! \brief A transform iterator, so we can expose the subnodes as a
    //! collection of nodes rather than \ref subnode_info objects.
    typedef boost::transform_iterator<subnode_transform_info, const_subnodeinfo_iterator> subnode_iterator;

    //! \copydoc node_impl::node_impl(const shared_db_ptr&,const node_info&)
    node(const shared_db_ptr& db, const node_info& info)
        : m_pimpl(new node_impl(db, info)) { }

    //! \brief Constructor for subnodes
    //!
    //! This constructor is specific to nodes defined in other nodes
    //! \param[in] container_node The parent or containing node
    //! \param[in] info Information about this node
    node(const node& container_node, const subnode_info& info)
        : m_pimpl(new node_impl(container_node.m_pimpl, info)) { }
    //! \copydoc node_impl::node_impl(const std::tr1::shared_ptr<node_impl>&,const subnode_info&)
    node(const std::tr1::shared_ptr<node_impl>& container_node, const subnode_info& info)
        : m_pimpl(new node_impl(container_node, info)) { }

    //! \brief Copy construct this node
    //! 
    //! This node will be another instance of the specified node
    //! \param[in] other The node to copy
    node(const node& other)
        : m_pimpl(new node_impl(*other.m_pimpl)) { }

    //! \brief Alias constructor
    //!
    //! This node will be an alias of the specified node. They will refer to the
    //! same in memory object - they share a \ref node_impl instance.
    //! \param[in] other The node to alias
    node(const node& other, alias_tag)
        : m_pimpl(other.m_pimpl) { }

#ifndef BOOST_NO_RVALUE_REFERENCES
    //! \brief Move constructor
    //! \param[in] other Node to move from
    node(node&& other)
        : m_pimpl(std::move(other.m_pimpl)) { }
#endif

    //! \copydoc node_impl::operator=()
    node& operator=(const node& other)
        { *m_pimpl = *(other.m_pimpl); return *this; }

    //! \copydoc node_impl::get_id()
    node_id get_id() const { return m_pimpl->get_id(); }
    //! \copydoc node_impl::get_data_id()
    block_id get_data_id() const { return m_pimpl->get_data_id(); }
    //! \copydoc node_impl::get_sub_id()
    block_id get_sub_id() const { return m_pimpl->get_sub_id(); }

    //! \copydoc node_impl::get_parent_id()
    node_id get_parent_id() const { return m_pimpl->get_parent_id(); } 
    //! \copydoc node_impl::is_subnode()
    bool is_subnode() { return m_pimpl->is_subnode(); } 

    //! \copydoc node_impl::get_data_block()
    std::tr1::shared_ptr<data_block> get_data_block() const
        { return m_pimpl->get_data_block(); }
    //! \copydoc node_impl::get_subnode_block()
    std::tr1::shared_ptr<subnode_block> get_subnode_block() const 
        { return m_pimpl->get_subnode_block(); } 
   
    //! \copydoc node_impl::read(std::vector<byte>&,ulong) const
    size_t read(std::vector<byte>& buffer, ulong offset) const
        { return m_pimpl->read(buffer, offset); }
    //! \copydoc node_impl::read(ulong) const
    template<typename T> T read(ulong offset) const
        { return m_pimpl->read<T>(offset); }
    //! \copydoc node_impl::read(std::vector<byte>&,uint,ulong) const
    size_t read(std::vector<byte>& buffer, uint page_num, ulong offset) const
        { return m_pimpl->read(buffer, page_num, offset); }
    //! \copydoc node_impl::read(uint,ulong) const
    template<typename T> T read(uint page_num, ulong offset) const
        { return m_pimpl->read<T>(page_num, offset); }

//! \cond write_api
    size_t write(std::vector<byte>& buffer, ulong offset) 
        { return m_pimpl->write(buffer, offset); }
    template<typename T> void write(const T& obj, ulong offset)
        { return m_pimpl->write<T>(obj, offset); }
    size_t write(std::vector<byte>& buffer, uint page_num, ulong offset) 
        { return m_pimpl->write(buffer, page_num, offset); }
    template<typename T> void write(const T& obj, uint page_num, ulong offset)
        { return m_pimpl->write<T>(obj, page_num, offset); }
    size_t resize(size_t size)
        { return m_pimpl->resize(size); }
//! \endcond

    //! \brief Creates a stream device over this node
    //!
    //! The returned stream device can be used to construct a proper stream:
    //! \code
    //! node n;
    //! node_stream nstream(n.open_as_stream());
    //! \endcode
    //! Which can then be used as any iostream would be.
    //! \returns A node stream device for this node
    node_stream_device open_as_stream()
        { return node_stream_device(m_pimpl); }

    //! \copydoc node_impl::size()
    size_t size() const { return m_pimpl->size(); }
    //! \copydoc node_impl::get_page_size()
    size_t get_page_size(uint page_num) const 
        { return m_pimpl->get_page_size(page_num); }
    //! \copydoc node_impl::get_page_count()
    uint get_page_count() const { return m_pimpl->get_page_count(); }

    // iterate over subnodes
    //! \copydoc node_impl::subnode_info_begin()
    const_subnodeinfo_iterator subnode_info_begin() const
        { return m_pimpl->subnode_info_begin(); }
    //! \copydoc node_impl::subnode_info_end()
    const_subnodeinfo_iterator subnode_info_end() const
        { return m_pimpl->subnode_info_end(); } 
    
    //! \brief Returns a proxy iterator for the first subnode
    //!
    //! This is known as a proxy iterator because the dereferenced type is
    //! of type node, not node& or const node&. This means the object is an
    //! rvalue, constructed specifically for the purpose of being returned
    //! from this iterator.
    //! \returns The proxy iterator
    subnode_iterator subnode_begin() const
        { return boost::make_transform_iterator(subnode_info_begin(), subnode_transform_info(m_pimpl)); }
 
    //! \brief Returns a proxy iterator positioned past the last subnode
    //!
    //! This is known as a proxy iterator because the dereferenced type is
    //! of type node, not node& or const node&. This means the object is an
    //! rvalue, constructed specifically for the purpose of being returned
    //! from this iterator.
    //! \returns The proxy iterator
    subnode_iterator subnode_end() const
        { return boost::make_transform_iterator(subnode_info_end(), subnode_transform_info(m_pimpl)); }

    //! \copydoc node_impl::lookup()
    node lookup(node_id id) const
        { return m_pimpl->lookup(id); }

private:
    std::tr1::shared_ptr<node_impl> m_pimpl; //!< Pointer to the node implementation
};

//! \defgroup ndb_blockrelated Blocks
//! \ingroup ndb

//! \brief The base class of the block class hierarchy
//!
//! A block is an atomic unit of storage in a PST file. This class, and other
//! classes in this hierarchy, are in memory representations of those blocks
//! on disk. They are immutable, and are shared freely between different node
//! instances as needed (via shared pointers). A block also knows how to read
//! any blocks it may refer to (in the case of extended_block or a 
//! subnode_block).
//!
//! All blocks in the block hierarchy are also in the category of what is known
//! as <i>dependant objects</i>. This means is they only keep a weak 
//! reference to the database context to which they're a member. Contrast this
//! to an independant object such as the \ref node, which keeps a strong ref
//! or a full shared_ptr to the related context. This implies that someone
//! must externally make sure the database context outlives it's blocks -
//! this is usually done by the database context itself or the node which
//! holds these blocks.
//! \sa disk_blockrelated
//! \sa [MS-PST] 2.2.2.8
//! \ingroup ndb_blockrelated
class block
{
public:
    //! \brief Basic block constructor
    //! \param[in] db The database context this block was opened in
    //! \param[in] info Information about this block
    block(const shared_db_ptr& db, const block_info& info)
        : m_modified(false), m_size(info.size), m_id(info.id), m_address(info.address), m_db(db) { }

//! \cond write_api
    block(const block& other)
        : m_modified(false), m_size(other.m_size), m_id(0), m_address(0), m_db(other.m_db) { }
//! \endcond

    virtual ~block() { }

    //! \brief Returns the blocks internal/external state
    //! \returns true if this is an internal block, false otherwise
    virtual bool is_internal() const = 0;

    //! \brief Get the last known size of this block on disk
    //! \returns The last known size of this block on disk
    size_t get_disk_size() const { return m_size; }
//! \cond write_api
    void set_disk_size(size_t new_size) { m_size = new_size; }    
//! \endcond

    //! \brief Get the block_id of this block
    //! \returns The block_id of this block
    block_id get_id() const { return m_id; }

    //! \brief Get the address of this block on disk
    //! \returns The address of this block, 0 for a new block.
    ulonglong get_address() const { return m_address; }
//! \cond write_api
    void set_address(ulonglong new_address) { m_address = new_address; }
    
    void touch();
//! \endcond

protected:
    shared_db_ptr get_db_ptr() const { return shared_db_ptr(m_db); }
    virtual void trim() { }

    bool m_modified;        //!< True if this block has been modified and needs to be saved
    size_t m_size;          //!< The size of this specific block on disk at last save
    block_id m_id;          //!< The id of this block
    ulonglong m_address;    //!< The address of this specific block on disk, 0 if unknown

    weak_db_ptr m_db;
};

//! \brief A block which represents end user data
//!
//! This class is the base class of both \ref extended_block and \ref
//! external_block. This base class exists to abstract away their 
//! differences, so a node can treat a given block (be it extended or
//! external) simply as a stream of bytes.
//! \ingroup ndb_blockrelated
class data_block : public block
{
public:
    //! \brief Constructor for a data_block
    //! \param[in] db The database context
    //! \param[in] info Information about this block
    //! \param[in] total_size The total logical size of this block
    data_block(const shared_db_ptr& db, const block_info& info, size_t total_size)
        : block(db, info), m_total_size(total_size) { }
    virtual ~data_block() { }

    //! \brief Read data from this block
    //!
    //! Fills the specified buffer with data starting at the specified offset.
    //! The size of the buffer indicates how much data to read.
    //! \param[in,out] buffer The buffer to fill
    //! \param[in] offset The location to read from
    //! \returns The amount of data read
    size_t read(std::vector<byte>& buffer, ulong offset) const;

    //! \brief Read data from this block
    //!
    //! Returns a "T" located as the specified offset
    //! \tparam T The type to read
    //! \param[in] offset The location to read from
    //! \returns The type read
    template<typename T> T read(ulong offset) const;

    //! \brief Read data from this block
    //! \param[out] pdest_buffer The location to read the data into
    //! \param[in] size The amount of data to read
    //! \param[in] offset The location to read from
    //! \returns The amount of data read
    virtual size_t read_raw(byte* pdest_buffer, size_t size, ulong offset) const = 0;

//! \cond write_api
    size_t write(const std::vector<byte>& buffer, ulong offset, std::tr1::shared_ptr<data_block>& presult);
    template<typename T> void write(const T& buffer, ulong offset, std::tr1::shared_ptr<data_block>& presult);
    virtual size_t write_raw(const byte* psrc_buffer, size_t size, ulong offset, std::tr1::shared_ptr<data_block>& presult) = 0;
//! \endcond

    //! \brief Get the number of physical pages in this data_block
    //! \note In this context, "page" refers to an external_block
    //! \returns The total number of external_blocks under this data_block
    virtual uint get_page_count() const = 0;

    //! \brief Get a specific page of this data_block
    //! \note In this context, "page" refers to an external_block
    //! \throws out_of_range If page_num >= get_page_count()
    //! \param[in] page_num The ordinal of the external_block to get, zero based
    //! \returns The requested external_block
    virtual std::tr1::shared_ptr<external_block> get_page(uint page_num) const = 0;

    //! \brief Get the total logical size of this block
    //! \returns The total logical size of this block
    size_t get_total_size() const { return m_total_size; }
//! \cond write_api
    virtual size_t resize(size_t size, std::tr1::shared_ptr<data_block>& presult) = 0;
//! \endcond

protected:
    size_t m_total_size;    //!< the total or logical size (sum of all external child blocks)
};

//! \brief A data block which refers to other data blocks, in order to extend
//! the physical size limit (8k) to a larger logical size.
//!
//! An extended_block is essentially a list of block_ids of other \ref 
//! data_block, which themselves may be an extended_block or an \ref
//! external_block. Ultimately they form a "data tree", the leafs of which
//! form the "logical" contents of the block.
//! 
//! This class is an in memory representation of the disk::extended_block
//! structure.
//! \sa [MS-PST] 2.2.2.8.3.2
//! \ingroup ndb_blockrelated
class extended_block : 
    public data_block, 
    public std::tr1::enable_shared_from_this<extended_block>
{
public:
    //! \brief Construct an extended_block from disk
    //! \param[in] db The database context
    //! \param[in] info Information about this block
    //! \param[in] level The level of this extended block (1 or 2)
    //! \param[in] total_size The total logical size of this block
    //! \param[in] child_max_total_size The maximum logical size of a child block
    //! \param[in] page_max_count The maximum number of external blocks that can be contained in this block
    //! \param[in] child_page_max_count The maximum number of external blocks that can be contained in a child block
    //! \param[in] bi The \ref block_info for all child blocks
#ifndef BOOST_NO_RVALUE_REFERENCES
    extended_block(const shared_db_ptr& db, const block_info& info, ushort level, size_t total_size, size_t child_max_total_size, ulong page_max_count, ulong child_page_max_count, std::vector<block_id> bi)
        : data_block(db, info, total_size), m_child_max_total_size(child_max_total_size), m_child_max_page_count(child_page_max_count), m_max_page_count(page_max_count), m_level(level), m_block_info(std::move(bi)), m_child_blocks(m_block_info.size()) { }
#else
    extended_block(const shared_db_ptr& db, const block_info& info, ushort level, size_t total_size, size_t child_max_total_size, ulong page_max_count, ulong child_page_max_count, const std::vector<block_id>& bi)
        : data_block(db, info, total_size), m_child_max_total_size(child_max_total_size), m_child_max_page_count(child_page_max_count), m_max_page_count(page_max_count), m_level(level), m_block_info(bi), m_child_blocks(m_block_info.size()) { }
#endif

//! \cond write_api
    // new block constructors
#ifndef BOOST_NO_RVALUE_REFERENCES
    extended_block(const shared_db_ptr& db, ushort level, size_t total_size, size_t child_max_total_size, ulong page_max_count, ulong child_page_max_count, std::vector<std::tr1::shared_ptr<data_block> > child_blocks)
        : data_block(db, block_info(), total_size), m_child_max_total_size(child_max_total_size), m_child_max_page_count(child_page_max_count), m_max_page_count(page_max_count), m_level(level), m_child_blocks(std::move(child_blocks))
        { m_block_info.resize(m_child_blocks.size()); touch(); }
#else
    extended_block(const shared_db_ptr& db, ushort level, size_t total_size, size_t child_max_total_size, ulong page_max_count, ulong child_page_max_count, const std::vector<std::tr1::shared_ptr<data_block> >& child_blocks)
        : data_block(db, block_info(), total_size), m_child_max_total_size(child_max_total_size), m_child_max_page_count(child_page_max_count), m_max_page_count(page_max_count), m_level(level), m_child_blocks(child_blocks)
        { m_block_info.resize(m_child_blocks.size()); touch(); }
#endif
    extended_block(const shared_db_ptr& db, ushort level, size_t total_size, size_t child_max_total_size, ulong page_max_count, ulong child_page_max_count);
//! \endcond

    size_t read_raw(byte* pdest_buffer, size_t size, ulong offset) const;
//! \cond write_api
    size_t write_raw(const byte* psrc_buffer, size_t size, ulong offset, std::tr1::shared_ptr<data_block>& presult);
//! \endcond
    
    uint get_page_count() const;
    std::tr1::shared_ptr<external_block> get_page(uint page_num) const;
    
//! \cond write_api
    size_t resize(size_t size, std::tr1::shared_ptr<data_block>& presult);
//! \endcond
    
    //! \brief Get the "level" of this extended_block
    //! 
    //! A level 1 extended_block (or "xblock") points to external blocks.
    //! A level 2 extended_block (or "xxblock") points to other extended_blocks
    //! \returns 1 for an xblock, 2 for an xxblock
    ushort get_level() const { return m_level; }
    bool is_internal() const { return true; }

private:
    extended_block& operator=(const extended_block& other); // = delete
    data_block* get_child_block(uint index) const;

    const size_t m_child_max_total_size;    //!< maximum (logical) size of a child block
    const ulong m_child_max_page_count;     //!< maximum number of child blocks a child can contain
    const ulong m_max_page_count;           //!< maximum number of child blocks this block can contain

    //! \brief Return the max logical size of this xblock
    size_t get_max_size() const { return m_child_max_total_size * m_max_page_count; }

    const ushort m_level;                   //!< The level of this block
    std::vector<block_id> m_block_info;     //!< block_ids of the child blocks in this tree
    mutable std::vector<std::tr1::shared_ptr<data_block> > m_child_blocks; //!< Cached child blocks
};

//! \brief Contains actual data
//!
//! An external_block contains the actual data contents used by the higher
//! layers. This data is also "encrypted", although the encryption/decryption
//! process occurs immediately before/after going to disk, not here.
//! \sa [MS-PST] 2.2.2.8.3.1
//! \ingroup ndb_blockrelated
class external_block : 
    public data_block, 
    public std::tr1::enable_shared_from_this<external_block>
{
public:
    //! \brief Construct an external_block from disk
    //! \param[in] db The database context
    //! \param[in] info Information about this block
    //! \param[in] max_size The maximum possible size of this block
    //! \param[in] buffer The actual external data (decoded)
#ifndef BOOST_NO_RVALUE_REFERENCES
    external_block(const shared_db_ptr& db, const block_info& info, size_t max_size, std::vector<byte> buffer)
        : data_block(db, info, info.size), m_max_size(max_size), m_buffer(std::move(buffer)) { }
#else
    external_block(const shared_db_ptr& db, const block_info& info, size_t max_size, const std::vector<byte>& buffer)
        : data_block(db, info, info.size), m_max_size(max_size), m_buffer(buffer) { }
#endif

//! \cond write_api
    // new block constructors
    external_block(const shared_db_ptr& db, size_t max_size, size_t current_size)
        : data_block(db, block_info(), current_size), m_max_size(max_size), m_buffer(current_size)
        { touch(); }
//! \endcond

    size_t read_raw(byte* pdest_buffer, size_t size, ulong offset) const;
//! \cond write_api
    size_t write_raw(const byte* psrc_buffer, size_t size, ulong offset, std::tr1::shared_ptr<data_block>& presult);
//! \endcond

    uint get_page_count() const { return 1; }
    std::tr1::shared_ptr<external_block> get_page(uint page_num) const;

//! \cond write_api
    size_t resize(size_t size, std::tr1::shared_ptr<data_block>& presult);
//! \endcond

    bool is_internal() const { return false; }

private:
    external_block& operator=(const external_block& other); // = delete

    const size_t m_max_size;
    size_t get_max_size() const { return m_max_size; }

    std::vector<byte> m_buffer;
};


//! \brief A block which contains information about subnodes
//!
//! Subnode blocks form a sort of "private NBT" for a node. This class is
//! the root class of that hierarchy, with child classes for the leaf and
//! non-leaf versions of a subnode block.
//!
//! This hierarchy also models the \ref btree_node structure, inheriting the 
//! actual iteration and lookup logic.
//! \sa [MS-PST] 2.2.2.8.3.3
//! \ingroup ndb_blockrelated
class subnode_block : 
    public block, 
    public virtual btree_node<node_id, subnode_info>
{
public:
    //! \brief Construct a block from disk
    //! \param[in] db The database context
    //! \param[in] info Information about this block
    //! \param[in] level The level of this subnode_block (0 or 1)
    subnode_block(const shared_db_ptr& db, const block_info& info, ushort level)
        : block(db, info), m_level(level) { }

    virtual ~subnode_block() { }

    //! \brief Get the level of this subnode_block
    //! \returns 0 for a leaf block, 1 otherwise
    ushort get_level() const { return m_level; }

    bool is_internal() const { return true; }
    
protected:
    ushort m_level; //!< Level of this subnode_block
};

//! \brief Contains references to subnode_leaf_blocks.
//!
//! Because of the width of a subnode_leaf_block and the relative scarcity of
//! subnodes, it's actually pretty uncommon to encounter a subnode non-leaf
//! block in practice. But it does occur, typically on large tables.
//!
//! This is the in memory version of one of these blocks. It forms the node
//! of a tree, similar to the NBT, pointing to child blocks. There can only
//! be one level of these - a subnode_nonleaf_block can not point to other
//! subnode_nonleaf_blocks.
//! \sa [MS-PST] 2.2.2.8.3.3.2
//! \ingroup ndb_blockrelated
class subnode_nonleaf_block : 
    public subnode_block, 
    public btree_node_nonleaf<node_id, subnode_info>, 
    public std::tr1::enable_shared_from_this<subnode_nonleaf_block>
{
public:
    //! \brief Construct a subnode_nonleaf_block from disk
    //! \param[in] db The database context
    //! \param[in] info Information about this block
    //! \param[in] subblocks Information about the child blocks
#ifndef BOOST_NO_RVALUE_REFERENCES
    subnode_nonleaf_block(const shared_db_ptr& db, const block_info& info, std::vector<std::pair<node_id, block_id> > subblocks)
        : subnode_block(db, info, 1), m_subnode_info(std::move(subblocks)), m_child_blocks(m_subnode_info.size()) { }
#else
    subnode_nonleaf_block(const shared_db_ptr& db, const block_info& info, const std::vector<std::pair<node_id, block_id> >& subblocks)
        : subnode_block(db, info, 1), m_subnode_info(subblocks), m_child_blocks(m_subnode_info.size()) { }
#endif

    // btree_node_nonleaf implementation
    const node_id& get_key(uint pos) const
        { return m_subnode_info[pos].first; }
    subnode_block* get_child(uint pos);
    const subnode_block* get_child(uint pos) const;
    uint num_values() const { return m_subnode_info.size(); }
    
private:
    std::vector<std::pair<node_id, block_id> > m_subnode_info;           //!< Info about the sub-blocks
    mutable std::vector<std::tr1::shared_ptr<subnode_block> > m_child_blocks; //!< Cached sub-blocks (leafs)
};

//! \brief Contains the actual subnode information
//!
//! Typically a node will point directly to one of these. Because they are
//! blocks, and thus up to 8k in size, they can hold information for about
//! ~300 subnodes in a unicode store, and up to ~600 in an ANSI store.
//! \sa [MS-PST] 2.2.2.8.3.3.1
//! \ingroup ndb_blockrelated
class subnode_leaf_block : 
    public subnode_block, 
    public btree_node_leaf<node_id, subnode_info>, 
    public std::tr1::enable_shared_from_this<subnode_leaf_block>
{
public:
    //! \brief Construct a subnode_leaf_block from disk
    //! \param[in] db The database context
    //! \param[in] info Information about this block
    //! \param[in] subnodes Information about the subnodes
#ifndef BOOST_NO_RVALUE_REFERENCES
    subnode_leaf_block(const shared_db_ptr& db, const block_info& info, std::vector<std::pair<node_id, subnode_info> > subnodes)
        : subnode_block(db, info, 0), m_subnodes(std::move(subnodes)) { }
#else
    subnode_leaf_block(const shared_db_ptr& db, const block_info& info, const std::vector<std::pair<node_id, subnode_info> >& subnodes)
        : subnode_block(db, info, 0), m_subnodes(subnodes) { }
#endif

    // btree_node_leaf implementation
    const subnode_info& get_value(uint pos) const 
        { return m_subnodes[pos].second; }
    const node_id& get_key(uint pos) const
        { return m_subnodes[pos].first; }
    uint num_values() const
        { return m_subnodes.size(); }

private:
    std::vector<std::pair<node_id, subnode_info> > m_subnodes;   //!< The actual subnode information
};

} // end pstsdk namespace

inline pstsdk::node pstsdk::subnode_transform_info::operator()(const pstsdk::subnode_info& info) const
{ 
    return node(m_parent, info); 
}

inline pstsdk::block_id pstsdk::node_impl::get_data_id() const
{ 
    if(m_pdata)
        return m_pdata->get_id();
    
    return m_original_data_id;
}

inline pstsdk::block_id pstsdk::node_impl::get_sub_id() const
{ 
    if(m_psub)
        return m_psub->get_id();
    
    return m_original_sub_id;
}

inline size_t pstsdk::node_impl::size() const
{
    return ensure_data_block()->get_total_size();
}

inline size_t pstsdk::node_impl::get_page_size(uint page_num) const
{
    return ensure_data_block()->get_page(page_num)->get_total_size();
}
    
inline pstsdk::uint pstsdk::node_impl::get_page_count() const 
{ 
    return ensure_data_block()->get_page_count(); 
}

inline size_t pstsdk::node_impl::read(std::vector<byte>& buffer, ulong offset) const
{ 
    return ensure_data_block()->read(buffer, offset); 
}

inline size_t pstsdk::node_impl::read_raw(byte* pdest_buffer, size_t size, ulong offset) const
{ 
    return ensure_data_block()->read_raw(pdest_buffer, size, offset); 
}

template<typename T> 
inline T pstsdk::node_impl::read(ulong offset) const
{
    return ensure_data_block()->read<T>(offset); 
}
    
inline size_t pstsdk::node_impl::read(std::vector<byte>& buffer, uint page_num, ulong offset) const
{ 
    return ensure_data_block()->get_page(page_num)->read(buffer, offset); 
}

template<typename T> 
inline T pstsdk::node_impl::read(uint page_num, ulong offset) const
{
    return ensure_data_block()->get_page(page_num)->read<T>(offset); 
}

//! \cond write_api
inline size_t pstsdk::node_impl::write(const std::vector<byte>& buffer, ulong offset)
{
    return ensure_data_block()->write(buffer, offset, m_pdata);
}

inline size_t pstsdk::node_impl::write_raw(const byte* pdest_buffer, size_t size, ulong offset)
{
    ensure_data_block();
    return m_pdata->write_raw(pdest_buffer, size, offset, m_pdata);
}

template<typename T> 
inline void pstsdk::node_impl::write(const T& obj, ulong offset)
{
    return ensure_data_block()->write<T>(obj, offset, m_pdata);
}

inline size_t pstsdk::node_impl::write(const std::vector<byte>& buffer, uint page_num, ulong offset)
{
    return ensure_data_block()->write(buffer, page_num * get_page_size(0) + offset, m_pdata);
}

template<typename T> 
inline void pstsdk::node_impl::write(const T& obj, uint page_num, ulong offset)
{
    return ensure_data_block()->write<T>(obj, page_num * get_page_size(0) + offset, m_pdata);
}

inline size_t pstsdk::node_impl::resize(size_t size)
{
    return ensure_data_block()->resize(size, m_pdata);
}
//! \endcond

inline pstsdk::data_block* pstsdk::node_impl::ensure_data_block() const
{ 
    if(!m_pdata) 
        m_pdata = m_db->read_data_block(m_original_data_id); 

    return m_pdata.get();
}
    
inline pstsdk::subnode_block* pstsdk::node_impl::ensure_sub_block() const
{ 
    if(!m_psub) 
        m_psub = m_db->read_subnode_block(m_original_sub_id); 

    return m_psub.get();
}

//! \cond write_api
inline void pstsdk::block::touch()
{ 
    if(!m_modified)
    {
        m_modified = true; 
        m_address = 0;
        m_size = 0;
        m_id = get_db_ptr()->alloc_bid(is_internal()); 
    }
}
//! \endcond

inline std::streamsize pstsdk::node_stream_device::read(char* pbuffer, std::streamsize n)
{
    size_t read = m_pnode->read_raw(reinterpret_cast<byte*>(pbuffer), static_cast<size_t>(n), static_cast<size_t>(m_pos));
    m_pos += read;

    if(read)
        return read;
    else
        return -1;
}

//! \cond write_api
inline std::streamsize pstsdk::node_stream_device::write(const char* pbuffer, std::streamsize n)
{
    size_t written = m_pnode->write_raw(reinterpret_cast<const byte*>(pbuffer), static_cast<size_t>(n), static_cast<size_t>(m_pos));
    m_pos += written;
    return written;
}
//! \endcond

inline std::streampos pstsdk::node_stream_device::seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way)
{
#if defined(_MSC_VER) && (_MSC_VER < 1600)
#pragma warning(push)
#pragma warning(disable:4244)
#endif
    if(way == std::ios_base::beg)
            m_pos = off;
    else if(way == std::ios_base::end)
        m_pos = m_pnode->size() + off;
    else
        m_pos += off;
#if defined(_MSC_VER) && (_MSC_VER < 1600)
#pragma warning(pop)
#endif

    if(m_pos < 0)
        m_pos = 0;
    else if(static_cast<size_t>(m_pos) > m_pnode->size())
        m_pos = m_pnode->size();

    return m_pos;
}

inline pstsdk::subnode_block* pstsdk::subnode_nonleaf_block::get_child(uint pos)
{
    if(m_child_blocks[pos] == NULL)
    {
        m_child_blocks[pos] = get_db_ptr()->read_subnode_block(m_subnode_info[pos].second);
    }

    return m_child_blocks[pos].get();
}

inline const pstsdk::subnode_block* pstsdk::subnode_nonleaf_block::get_child(uint pos) const
{
    if(m_child_blocks[pos] == NULL)
    {
        m_child_blocks[pos] = get_db_ptr()->read_subnode_block(m_subnode_info[pos].second);
    }

    return m_child_blocks[pos].get();
}

inline size_t pstsdk::data_block::read(std::vector<byte>& buffer, ulong offset) const
{
    size_t read_size = buffer.size();
    
    if(read_size > 0)
    {
        if(offset >= get_total_size())
            throw std::out_of_range("offset >= size()");

        read_size = read_raw(&buffer[0], read_size, offset);
    }

    return read_size;
}

template<typename T> 
inline T pstsdk::data_block::read(ulong offset) const
{
    if(offset >= get_total_size())
        throw std::out_of_range("offset >= size()");
    if(sizeof(T) + offset > get_total_size())
        throw std::out_of_range("sizeof(T) + offset >= size()");

    T t;
    read_raw(reinterpret_cast<byte*>(&t), sizeof(T), offset);

    return t;
}

//! \cond write_api
inline size_t pstsdk::data_block::write(const std::vector<byte>& buffer, ulong offset, std::tr1::shared_ptr<data_block>& presult)
{
    size_t write_size = buffer.size();
    
    if(write_size > 0)
    {
        if(offset >= get_total_size())
            throw std::out_of_range("offset >= size()");

        write_size = write_raw(&buffer[0], write_size, offset, presult);
    }

    return write_size;
}

template<typename T> 
void pstsdk::data_block::write(const T& buffer, ulong offset, std::tr1::shared_ptr<data_block>& presult)
{
    if(offset >= get_total_size())
        throw std::out_of_range("offset >= size()");
    if(sizeof(T) + offset > get_total_size())
        throw std::out_of_range("sizeof(T) + offset >= size()");

    (void)write_raw(reinterpret_cast<const byte*>(&buffer), sizeof(T), offset, presult);
}
//! \endcond

inline pstsdk::uint pstsdk::extended_block::get_page_count() const
{
    assert(m_child_max_total_size % m_child_max_page_count == 0);
    uint page_size = m_child_max_total_size / m_child_max_page_count;
    uint page_count = (get_total_size() / page_size) + ((get_total_size() % page_size) != 0 ? 1 : 0);
    assert(get_level() == 2 || page_count == m_block_info.size());

    return page_count;
}

//! \cond write_api
inline pstsdk::extended_block::extended_block(const shared_db_ptr& db, ushort level, size_t total_size, size_t child_max_total_size, ulong page_max_count, ulong child_page_max_count)
: data_block(db, block_info(), total_size), m_child_max_total_size(child_max_total_size), m_child_max_page_count(child_page_max_count), m_max_page_count(page_max_count), m_level(level)
{
    int total_subblocks = total_size / m_child_max_total_size;
    if(total_size % m_child_max_total_size != 0)
        total_subblocks++;

    m_child_blocks.resize(total_subblocks);
    m_block_info.resize(total_subblocks, 0);

    touch();
}
//! \endcond

inline pstsdk::data_block* pstsdk::extended_block::get_child_block(uint index) const
{
    if(index >= m_child_blocks.size())
        throw std::out_of_range("index >= m_child_blocks.size()");

    if(m_child_blocks[index] == NULL)
    {
        if(m_block_info[index] == 0)
        {
            if(get_level() == 1)
                m_child_blocks[index] = get_db_ptr()->create_external_block(m_child_max_total_size);
            else
                m_child_blocks[index] = get_db_ptr()->create_extended_block(m_child_max_total_size);
        }
        else
            m_child_blocks[index] = get_db_ptr()->read_data_block(m_block_info[index]);
    }

    return m_child_blocks[index].get();
}

inline std::tr1::shared_ptr<pstsdk::external_block> pstsdk::extended_block::get_page(uint page_num) const
{
    uint page = page_num / m_child_max_page_count;
    return get_child_block(page)->get_page(page_num % m_child_max_page_count);
}

inline std::tr1::shared_ptr<pstsdk::external_block> pstsdk::external_block::get_page(uint index) const
{
    if(index != 0)
        throw std::out_of_range("index > 0");

    return std::tr1::const_pointer_cast<external_block>(this->shared_from_this());
}

inline size_t pstsdk::external_block::read_raw(byte* pdest_buffer, size_t size, ulong offset) const
{
    size_t read_size = size;

    assert(offset <= get_total_size());

    if(offset + size > get_total_size())
        read_size = get_total_size() - offset;

    memcpy(pdest_buffer, &m_buffer[offset], read_size);

    return read_size;
}

//! \cond write_api
inline size_t pstsdk::external_block::write_raw(const byte* psrc_buffer, size_t size, ulong offset, std::tr1::shared_ptr<data_block>& presult)
{
    std::tr1::shared_ptr<pstsdk::external_block> pblock = shared_from_this();
    if(pblock.use_count() > 2) // one for me, one for the caller
    {
        std::tr1::shared_ptr<pstsdk::external_block> pnewblock(new external_block(*this));
        return pnewblock->write_raw(psrc_buffer, size, offset, presult);
    }
    touch(); // mutate ourselves inplace

    assert(offset <= get_total_size());

    size_t write_size = size;

    if(offset + size > get_total_size())
        write_size = get_total_size() - offset;

    memcpy(&m_buffer[0]+offset, psrc_buffer, write_size);

    // assign out param
#ifndef BOOST_NO_RVALUE_REFERENCES
    presult = std::move(pblock);
#else
    presult = pblock;
#endif

    return write_size;
}
//! \endcond

inline size_t pstsdk::extended_block::read_raw(byte* pdest_buffer, size_t size, ulong offset) const
{
    assert(offset <= get_total_size());

    if(offset + size > get_total_size())
        size = get_total_size() - offset;

    byte* pend = pdest_buffer + size;

    size_t total_bytes_read = 0;

    while(pdest_buffer != pend)
    {
        // the child this read starts on
        uint child_pos = offset / m_child_max_total_size;
        // offset into the child block this read starts on
        ulong child_offset = offset % m_child_max_total_size;

        // call into our child to read the data
        size_t bytes_read = get_child_block(child_pos)->read_raw(pdest_buffer, size, child_offset);
        assert(bytes_read <= size);
    
        // adjust pointers accordingly
        pdest_buffer += bytes_read;
        offset += bytes_read;
        size -= bytes_read;
        total_bytes_read += bytes_read;

        assert(pdest_buffer <= pend);
    }

    return total_bytes_read;
}

//! \cond write_api
inline size_t pstsdk::extended_block::write_raw(const byte* psrc_buffer, size_t size, ulong offset, std::tr1::shared_ptr<data_block>& presult)
{
    std::tr1::shared_ptr<extended_block> pblock = shared_from_this();
    if(pblock.use_count() > 2) // one for me, one for the caller
    {
        std::tr1::shared_ptr<extended_block> pnewblock(new extended_block(*this));
        return pnewblock->write_raw(psrc_buffer, size, offset, presult);
    }
    touch(); // mutate ourselves inplace

    assert(offset <= get_total_size());

    if(offset + size > get_total_size())
        size = get_total_size() - offset;

    const byte* pend = psrc_buffer + size;
    size_t total_bytes_written = 0;

    while(psrc_buffer != pend)
    {
        // the child this read starts on
        uint child_pos = offset / m_child_max_total_size;
        // offset into the child block this read starts on
        ulong child_offset = offset % m_child_max_total_size;

        // call into our child to write the data
        size_t bytes_written = get_child_block(child_pos)->write_raw(psrc_buffer, size, child_offset, m_child_blocks[child_pos]);
        assert(bytes_written <= size);
    
        // adjust pointers accordingly
        psrc_buffer += bytes_written;
        offset += bytes_written;
        size -= bytes_written;
        total_bytes_written += bytes_written;

        assert(psrc_buffer <= pend);
    }

    // assign out param
#ifndef BOOST_NO_RVALUE_REFERENCES
    presult = std::move(pblock);
#else
    presult = pblock;
#endif

    return total_bytes_written;
}

inline size_t pstsdk::external_block::resize(size_t size, std::tr1::shared_ptr<data_block>& presult)
{
    std::tr1::shared_ptr<external_block> pblock = shared_from_this();
    if(pblock.use_count() > 2) // one for me, one for the caller
    {
        std::tr1::shared_ptr<external_block> pnewblock(new external_block(*this));
        return pnewblock->resize(size, presult);
    }
    touch(); // mutate ourselves inplace

    m_buffer.resize(size > m_max_size ? m_max_size : size);
    m_total_size = m_buffer.size();

    if(size > get_max_size())
    {
        // we need to create an extended_block with us as the first entry
        std::tr1::shared_ptr<extended_block> pnewxblock = get_db_ptr()->create_extended_block(pblock);
        return pnewxblock->resize(size, presult);
    }

    // assign out param
#ifndef BOOST_NO_RVALUE_REFERENCES
    presult = std::move(pblock);
#else
    presult = pblock;
#endif

    return size;
}

inline size_t pstsdk::extended_block::resize(size_t size, std::tr1::shared_ptr<data_block>& presult)
{
    // calculate the number of subblocks needed
    uint old_num_subblocks = m_block_info.size();
    uint num_subblocks = size / m_child_max_total_size;
    
    if(size % m_child_max_total_size != 0)
        num_subblocks++;

    if(num_subblocks > m_max_page_count)
        num_subblocks = m_max_page_count;

    // defer to child if it's 1 (or less)
    assert(!m_child_blocks.empty());
    if(num_subblocks < 2)
        return get_child_block(0)->resize(size, presult);

    std::tr1::shared_ptr<extended_block> pblock = shared_from_this();
    if(pblock.use_count() > 2) // one for me, one for the caller
    {
        std::tr1::shared_ptr<extended_block> pnewblock(new extended_block(*this));
        return pnewblock->resize(size, presult);
    }
    touch(); // mutate ourselves inplace

    // set the total number of subblocks needed
    m_block_info.resize(num_subblocks, 0);
    m_child_blocks.resize(num_subblocks);

    if(old_num_subblocks < num_subblocks)
        get_child_block(old_num_subblocks-1)->resize(m_child_max_total_size, m_child_blocks[old_num_subblocks-1]);

    // size the last subblock appropriately
    size_t last_child_size = size - (num_subblocks-1) * m_child_max_total_size;
    get_child_block(num_subblocks-1)->resize(last_child_size, m_child_blocks[num_subblocks-1]);

    if(size > get_max_size())
    {
        m_total_size = get_max_size();

        if(get_level() == 2)
            throw can_not_resize("size > max_size");

        // we need to create a level 2 extended_block with us as the first entry
        std::tr1::shared_ptr<extended_block> pnewxblock = get_db_ptr()->create_extended_block(pblock);
        return pnewxblock->resize(size, presult);
    }
    
    // assign out param
    m_total_size = size;
#ifndef BOOST_NO_RVALUE_REFERENCES
    presult = std::move(pblock);
#else
    presult = pblock;
#endif

    return size;
}
//! \endcond

inline pstsdk::const_subnodeinfo_iterator pstsdk::node_impl::subnode_info_begin() const
{
    const subnode_block* pblock = ensure_sub_block();
    return pblock->begin();
}

inline pstsdk::const_subnodeinfo_iterator pstsdk::node_impl::subnode_info_end() const
{
    const subnode_block* pblock = ensure_sub_block();
    return pblock->end();
}

inline pstsdk::node pstsdk::node_impl::lookup(node_id id) const
{
    return node(std::tr1::const_pointer_cast<node_impl>(shared_from_this()), ensure_sub_block()->lookup(id));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
