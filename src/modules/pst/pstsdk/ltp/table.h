//! \file
//! \brief Table (or Table Context, or TC) implementation
//! \author Terry Mahaffey
//! \ingroup ltp

#ifndef PSTSDK_LTP_TABLE_H
#define PSTSDK_LTP_TABLE_H

#include <vector>
#if __GNUC__
# include <tr1/unordered_map>
#else
# include <unordered_map>
#endif
#include <boost/iterator/iterator_facade.hpp>

#include "pstsdk/util/primitives.h"

#include "pstsdk/ndb/node.h"

#include "pstsdk/ltp/object.h"
#include "pstsdk/ltp/heap.h"

namespace pstsdk
{

class table_impl;
//! \addtogroup ltp_objectrelated
//@{
typedef std::tr1::shared_ptr<table_impl> table_ptr;
typedef std::tr1::shared_ptr<const table_impl> const_table_ptr;
//@}

//! \brief Open the specified node as a table
//! \param[in] n The node to copy and interpret as a TC
//! \ingroup ltp_objectrelated
table_ptr open_table(const node& n);
//! \brief Open the specified node as a table
//! \param[in] n The node to alias and interpret as a TC
//! \ingroup ltp_objectrelated
table_ptr open_table(const node&, alias_tag);

//! \brief An abstraction of a table row
//!
//! A const_table_row represents a single row in a table. It models
//! a \ref const_property_object, allowing access to the properties
//! stored in the row.
//! 
//! This object is basically a thin wrapper around the table proper,
//! it holds a reference to the table as well as the index of the 
//! row it represents, and fowards most requests to the table.
//! \ingroup ltp_objectrelated
class const_table_row : public const_property_object
{
public:
    //! \brief Copy construct this row
    //! \param[in] other The row to copy from
    const_table_row(const const_table_row& other)
        : m_position(other.m_position), m_table(other.m_table) { }
    //! \brief Construct a const_table_row object from a table and row offset
    //! \param[in] position The offset into the table to represent
    //! \param[in] table The table to reference
    const_table_row(ulong position, const const_table_ptr& table)
        : m_position(position), m_table(table) { }

    //! \brief Get the row_id of this row
    //! \returns The row_id of this row
    row_id get_row_id() const;

    // const_property_object
    std::vector<prop_id> get_prop_list() const;
    prop_type get_prop_type(prop_id id) const;
    bool prop_exists(prop_id id) const;
    size_t size(prop_id id) const;
    hnid_stream_device open_prop_stream(prop_id id);

private:
    // const_property_object
    byte get_value_1(prop_id id) const;
    ushort get_value_2(prop_id id) const;
    ulong get_value_4(prop_id id) const;
    ulonglong get_value_8(prop_id id) const;
    std::vector<byte> get_value_variable(prop_id id) const;

    ulong m_position;           //!< The row this object represents
    const_table_ptr m_table;    //!< The table this object is a part of
};

//! \brief The iterator type exposed by the table for row iteration
//!
//! The iterator type is built using the boost iterator library. It is a 
//! random access proxy iterator; which constructs a const_table_row
//! from a position offset and table reference, allowing property access
//! to the row.
//! \ingroup ltp_objectrelated
class const_table_row_iter : public boost::iterator_facade<const_table_row_iter, const_table_row, boost::random_access_traversal_tag, const_table_row>
{
public:
    //! \brief Default constructor
    const_table_row_iter()
        : m_position(0) { }

    //! \brief Construct an iterator from a position and table
    //! \param[in] pos The offset into the table
    //! \param[in] table The table
    const_table_row_iter(ulong pos, const const_table_ptr& table) 
        : m_position(pos), m_table(table)  { }

private:
    friend class boost::iterator_core_access;

    void increment() { ++m_position; }
    bool equal(const const_table_row_iter& other) const
        { return ((m_position == other.m_position) && (m_table == other.m_table)); }
    const_table_row dereference() const
        { return const_table_row(m_position, m_table); }
    void decrement() { --m_position; }
    void advance(int off) { m_position += off; }
    size_t distance_to(const const_table_row_iter& other) const
        { return (other.m_position - m_position); }

    ulong m_position;
    const_table_ptr m_table;
};

//! \brief Table implementation
//!
//! Similar to the \ref node and \ref heap classes, the table class is divided
//! into an implementation class and stack based class. Child objects (in this
//! case, table rows) can reference the implementation class and thus safely
//! outlive the stack based class.
//!
//! A key difference in the table implementation, however, is that there are
//! actually a few different types of tables. So instead of having one impl
//! class, we have one impl base class to which everyone holds a shared pointer
//! to, and several child classes which we instiate depending on the actual
//! underlying table type. This is the table implementation "interface" class.
//! \sa [MS-PST] 2.3.4
//! \ingroup ltp_objectrelated
class table_impl : public std::tr1::enable_shared_from_this<table_impl>
{
public:
    virtual ~table_impl() { }
    //! \brief Find the offset into the table of the given row_id
    //! \throws key_not_found<row_id> If the given row_id is not present in this table
    //! \param[in] id The row id to lookup
    //! \returns The offset into the table
    virtual ulong lookup_row(row_id id) const = 0;

    //! \brief Get the requested table row
    //! \param[in] row The offset into the table to construct a row for
    //! \returns The requested row
    const_table_row operator[](ulong row) const
        { return const_table_row(row, shared_from_this()); }
    //! \brief Get an iterator pointing to the first row
    //! \returns The requested iterator
    const_table_row_iter begin() const
        { return const_table_row_iter(0, shared_from_this()); }
    //! \brief Get an end iterator for this table
    //! \returns The requested iterator
    const_table_row_iter end() const
        { return const_table_row_iter(size(), shared_from_this()); }
    
    //! \brief Get the node backing this table
    //! \returns The node
    virtual node& get_node() = 0;
    //! \brief Get the node backing this table
    //! \returns The node
    virtual const node& get_node() const = 0;
    //! \brief Get the contents of the specified cell in the specified row
    //! \throws key_not_found<prop_id> If the specified property does not exist on the specified row
    //! \throws out_of_range If the specified row offset is beyond the size of this table
    //! \param[in] row The offset into the table
    //! \param[in] id The prop_id to find the cell value of
    //! \returns The cell value
    virtual ulonglong get_cell_value(ulong row, prop_id id) const = 0;
    //! \brief Get the contents of a indirect property in the specified row
    //! \throws key_not_found<prop_id> If the specified property does not exist on the specified row
    //! \throws out_of_range If the specified row offset is beyond the size of this table
    //! \param[in] row The offset into the table
    //! \param[in] id The prop_id to find the cell value of
    //! \returns The raw bytes of the property
    virtual std::vector<byte> read_cell(ulong row, prop_id id) const = 0;
    //! \brief Open a stream over a property in a given row
    //! \throws key_not_found<prop_id> If the specified property does not exist on the specified row
    //! \throws out_of_range If the specified row offset is beyond the size of this table
    //! \note This operation is only valid for variable length properties
    //! \param[in] row The offset into the table
    //! \param[in] id The prop_id to find the cell value of
    //! \returns A device which can be used to construct a stream object
    virtual hnid_stream_device open_cell_stream(ulong row, prop_id id) = 0;
    //! \brief Get all of the properties on this table
    //!
    //! This returns the properties behind all of the columns which make up
    //! this table. This doesn't imply that every property is present on every
    //! row, however.
    //! \returns A vector all of the column prop_ids
    virtual std::vector<prop_id> get_prop_list() const = 0;
    //! \brief Get the type of a property
    //! \returns The property type
    virtual prop_type get_prop_type(prop_id id) const = 0;
    //! \brief Get the row id of a specified row
    //! 
    //! On disk, the first DWORD is always the row_id.
    //! \sa [MS-PST] 2.3.4.4.1/dwRowID
    //! \returns The row id
    virtual row_id get_row_id(ulong row) const = 0;
    //! \brief Get the number of rows in this table
    //! \returns The number of rows
    virtual size_t size() const = 0;
    //! \brief Check to see if a property exists for a given row
    //! \param[in] row The offset into the table
    //! \param[in] id The prop_id
    //! \returns true if the property exists
    virtual bool prop_exists(ulong row, prop_id id) const = 0;
    //! \brief Return the size of a property for a given row
    //! \note This operation is only valid for variable length properties
    //! \param[in] row The offset into the table
    //! \param[in] id The prop_id
    //! \returns The vector.size() if read_prop were called
    virtual size_t row_prop_size(ulong row, prop_id id) const = 0;
};

//! \brief Implementation of an ANSI TC (64k rows) and a unicode TC
//!
//! ANSI and Unicode TCs differ in the "row index" BTH. On an ANSI PST
//! the type type is 2 bytes and in Unicode PSTs the value type is 4
//! bytes. Since the value type is the row offset for a given row_id,
//! that implies a 64k row limit for TCs in ANSI stores.
//!
//! Note that the information about the key/value size is stored in the
//! BTH header. We use that to determine what type of table this is, rather
//! than trying to figure out if we're opened over an ANSI or Unicode PST
//! (which has been abstracted away from us at this point).
//! \tparam T The size of the value type in the row index BTH - ushort for an ANSI table, ulong for a Unicode table
//! \sa [MS-PST] 2.3.4.3.1/dwRowIndex
//! \ingroup ltp_objectrelated
template<typename T>
class basic_table : public table_impl
{
public:
    node& get_node() 
        { return m_prows->get_node(); }
    const node& get_node() const
        { return m_prows->get_node(); }
    ulong lookup_row(row_id id) const;
    ulonglong get_cell_value(ulong row, prop_id id) const;
    std::vector<byte> read_cell(ulong row, prop_id id) const;
    hnid_stream_device open_cell_stream(ulong row, prop_id id);
    std::vector<prop_id> get_prop_list() const;
    prop_type get_prop_type(prop_id id) const;
    row_id get_row_id(ulong row) const;
    size_t size() const;
    bool prop_exists(ulong row, prop_id id) const;
    size_t row_prop_size(ulong row, prop_id id) const;

private:
    friend table_ptr open_table(const node& n);
    friend table_ptr open_table(const node& n, alias_tag);
    basic_table(const node& n);
    basic_table(const node& n, alias_tag);

    std::tr1::shared_ptr<bth_node<row_id, T> > m_prows;

    // only one of the following two items is valid
    std::vector<byte> m_vec_rowarray;
    std::tr1::shared_ptr<node> m_pnode_rowarray;

    std::tr1::unordered_map<prop_id, disk::column_description> m_columns; 
    typedef std::tr1::unordered_map<prop_id, disk::column_description>::iterator column_iter;
    typedef std::tr1::unordered_map<prop_id, disk::column_description>::const_iterator const_column_iter;

    ushort m_offsets[disk::tc_offsets_max];

    // helper functions
    //! \brief Calculate the number of bytes per row
    //! \returns The number of bytes for a single row
    ulong cb_per_row() const { return m_offsets[disk::tc_offsets_bitmap]; }
    //! \brief Get the offset of the CEB (cell existance bitmap)
    //! \returns The offset into a row of the CEB
    ulong exists_bitmap_start() const { return m_offsets[disk::tc_offsets_one]; }
    //! \brief Calculate the number of rows per page (..external block)
    //! \returns The number of rows which fit on a single external block
    ulong rows_per_page() const { return (m_pnode_rowarray ? m_pnode_rowarray->get_page_size(0) / cb_per_row() : m_vec_rowarray.size() / cb_per_row()); }
    //! \brief Read and interpret data from a row
    //! \tparam Val the type to read
    //! \param[in] row The row to read
    //! \param[in] offset The offset into the row
    template<typename Val> Val read_raw_row(ulong row, ushort offset) const;
    //! \brief Read the CEB for a given row
    //! \returns The CEB
    std::vector<byte> read_exists_bitmap(ulong row) const;
};

typedef basic_table<ushort> small_table;
typedef basic_table<ulong> large_table;

//! \brief The actual table object that clients reference
//!
//! The table object is an in memory representation of the table context (TC).
//! Clients use the \ref open_table(const node&) free function to create table
//! objects. 
//!
//! A table object's job in general is to allow access to the individual row
//! objects, either via operator[] or the iterators. Most property access should
//! go through the row objects. Other member functions allow for row and type
//! lookup.
//! \sa [MS-PST] 2.3.4
//! \ingroup ltp_objectrelated
class table
{
public:
    //! \brief Construct a table from this node
    //! \param[in] n The node to copy and interpret as a table
    explicit table(const node& n);
    //! \brief Construct a table from this node
    //! \param[in] n The node to alias and interpret as a table
    table(const node& n, alias_tag);
    //! \brief Copy constructor
    //! \param[in] other The table to copy
    table(const table& other);
    //! \brief Alias constructor
    //! \param[in] other The table to alias
    table(const table& other, alias_tag)
        : m_ptable(other.m_ptable) { }

    //! \copydoc table_impl::operator[]()
    const_table_row operator[](ulong row) const
        { return (*m_ptable)[row]; }
    //! \copydoc table_impl::begin()
    const_table_row_iter begin() const
        { return m_ptable->begin(); }
    //! \copydoc table_impl::end()
    const_table_row_iter end() const
        { return m_ptable->end(); }

    //! \copydoc table_impl::get_node()
    node& get_node() 
        { return m_ptable->get_node(); }
    //! \copydoc table_impl::get_node() const
    const node& get_node() const
        { return m_ptable->get_node(); }
    //! \copydoc table_impl::get_cell_value()
    ulonglong get_cell_value(ulong row, prop_id id) const
        { return m_ptable->get_cell_value(row, id); }
    //! \copydoc table_impl::read_cell()
    std::vector<byte> read_cell(ulong row, prop_id id) const
        { return m_ptable->read_cell(row, id); }
    //! \copydoc table_impl::open_cell_stream()
    hnid_stream_device open_cell_stream(ulong row, prop_id id)
        { return m_ptable->open_cell_stream(row, id); }
    //! \copydoc table_impl::get_prop_list()
    std::vector<prop_id> get_prop_list() const
        { return m_ptable->get_prop_list(); }
    //! \copydoc table_impl::get_prop_type()
    prop_type get_prop_type(prop_id id) const
        { return m_ptable->get_prop_type(id); }
    //! \copydoc table_impl::get_row_id()
    row_id get_row_id(ulong row) const
        { return m_ptable->get_row_id(row); }
    //! \copydoc table_impl::lookup_row()
    ulong lookup_row(row_id id) const
        { return m_ptable->lookup_row(id); }
    //! \copydoc table_impl::size()
    size_t size() const
        { return m_ptable->size(); }
private:
    table();

    table_ptr m_ptable;
};

} // end pstsdk namespace

inline pstsdk::table_ptr pstsdk::open_table(const node& n)
{
    if(n.get_id() == nid_all_message_search_contents)
    {
        //return table_ptr(new gust(n));
        throw not_implemented("gust table");
    }

    heap h(n);
    std::vector<byte> table_info = h.read(h.get_root_id());
    disk::tc_header* pheader = (disk::tc_header*)&table_info[0];

    std::vector<byte> bth_info = h.read(pheader->row_btree_id);
    disk::bth_header* pbthheader = (disk::bth_header*)&bth_info[0];

    if(pbthheader->entry_size == 4)
       return table_ptr(new large_table(n));
    else
       return table_ptr(new small_table(n));
}

inline pstsdk::table_ptr pstsdk::open_table(const node& n, alias_tag)
{
    if(n.get_id() == nid_all_message_search_contents)
    {
        //return table_ptr(new gust(n));
        throw not_implemented("gust table");
    }

    heap h(n);
    std::vector<byte> table_info = h.read(h.get_root_id());
    disk::tc_header* pheader = (disk::tc_header*)&table_info[0];

    std::vector<byte> bth_info = h.read(pheader->row_btree_id);
    disk::bth_header* pbthheader = (disk::bth_header*)&bth_info[0];

    if(pbthheader->entry_size == 4)
       return table_ptr(new large_table(n, alias_tag()));
    else
       return table_ptr(new small_table(n, alias_tag()));
}

inline std::vector<pstsdk::prop_id> pstsdk::const_table_row::get_prop_list() const
{
    std::vector<prop_id> columns = m_table->get_prop_list();
    std::vector<prop_id> props;

    for(size_t i = 0; i < columns.size(); ++i)
    {
        if(prop_exists(columns[i]))
            props.push_back(columns[i]);
    }

    return props;
}

inline size_t pstsdk::const_table_row::size(prop_id id) const
{
    return m_table->row_prop_size(m_position, id);
}

inline pstsdk::prop_type pstsdk::const_table_row::get_prop_type(prop_id id) const
{
    return m_table->get_prop_type(id);
}

inline bool pstsdk::const_table_row::prop_exists(prop_id id) const
{
    return m_table->prop_exists(m_position, id);
}

inline pstsdk::row_id pstsdk::const_table_row::get_row_id() const
{
    return m_table->get_row_id(m_position);
}

inline pstsdk::byte pstsdk::const_table_row::get_value_1(prop_id id) const
{
    return (byte)m_table->get_cell_value(m_position, id); 
}

inline pstsdk::ushort pstsdk::const_table_row::get_value_2(prop_id id) const
{
    return (ushort)m_table->get_cell_value(m_position, id); 
}

inline pstsdk::ulong pstsdk::const_table_row::get_value_4(prop_id id) const
{
    return (ulong)m_table->get_cell_value(m_position, id); 
}

inline pstsdk::ulonglong pstsdk::const_table_row::get_value_8(prop_id id) const
{
    return m_table->get_cell_value(m_position, id); 
}

inline std::vector<pstsdk::byte> pstsdk::const_table_row::get_value_variable(prop_id id) const
{ 
    return m_table->read_cell(m_position, id); 
}

inline pstsdk::hnid_stream_device pstsdk::const_table_row::open_prop_stream(prop_id id)
{
    return (std::tr1::const_pointer_cast<table_impl>(m_table))->open_cell_stream(m_position, id);
}

template<typename T>
inline pstsdk::basic_table<T>::basic_table(const node& n)
{
    heap h(n, disk::heap_sig_tc);

    std::vector<byte> table_info = h.read(h.get_root_id());
    disk::tc_header* pheader = (disk::tc_header*)&table_info[0];

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(pheader->signature != disk::heap_sig_tc)
        throw sig_mismatch("heap_sig_tc expected", 0, n.get_id(), pheader->signature, disk::heap_sig_tc);
#endif

    m_prows = h.open_bth<row_id, T>(pheader->row_btree_id);

    for(int i = 0; i < pheader->num_columns; ++i)
        m_columns[pheader->columns[i].id] = pheader->columns[i];

    for(int i = 0; i < disk::tc_offsets_max; ++i)
        m_offsets[i] = pheader->size_offsets[i];

    if(is_subnode_id(pheader->row_matrix_id))
    {
        m_pnode_rowarray.reset(new node(n.lookup(pheader->row_matrix_id)));
    }
    else if(pheader->row_matrix_id)
    {
        m_vec_rowarray = h.read(pheader->row_matrix_id);
    }
}

template<typename T>
inline pstsdk::basic_table<T>::basic_table(const node& n, alias_tag)
{
    heap h(n, disk::heap_sig_tc, alias_tag());

    std::vector<byte> table_info = h.read(h.get_root_id());
    disk::tc_header* pheader = (disk::tc_header*)&table_info[0];

#ifdef PSTSDK_VALIDATION_LEVEL_WEAK
    if(pheader->signature != disk::heap_sig_tc)
        throw sig_mismatch("heap_sig_tc expected", 0, n.get_id(), pheader->signature, disk::heap_sig_tc);
#endif

    m_prows = h.open_bth<row_id, T>(pheader->row_btree_id);

    for(int i = 0; i < pheader->num_columns; ++i)
        m_columns[pheader->columns[i].id] = pheader->columns[i];

    for(int i = 0; i < disk::tc_offsets_max; ++i)
        m_offsets[i] = pheader->size_offsets[i];

    if(is_subnode_id(pheader->row_matrix_id))
    {
        m_pnode_rowarray.reset(new node(n.lookup(pheader->row_matrix_id)));
    }
    else if(pheader->row_matrix_id)
    {
        m_vec_rowarray = h.read(pheader->row_matrix_id);
    }
}

template<typename T>
inline size_t pstsdk::basic_table<T>::size() const
{
    if(m_pnode_rowarray)
    {
        return (m_pnode_rowarray->get_page_count()-1) * rows_per_page() + m_pnode_rowarray->get_page_size(m_pnode_rowarray->get_page_count()-1) / cb_per_row();
    }
    else 
    {
        return m_vec_rowarray.size() / cb_per_row();
    }
}

template<typename T>
inline std::vector<pstsdk::prop_id> pstsdk::basic_table<T>::get_prop_list() const
{
    std::vector<prop_id> props;

    for(const_column_iter i = m_columns.begin(); i != m_columns.end(); ++i)
        props.push_back(i->first);

    return props;
}

template<typename T>
inline pstsdk::ulong pstsdk::basic_table<T>::lookup_row(row_id id) const
{ 
    try 
    { 
        return (ulong)m_prows->lookup(id); 
    } 
    catch(key_not_found<T>& e) 
    { 
        throw key_not_found<row_id>(e.which()); 
    } 
}

template<typename T>
inline pstsdk::ulonglong pstsdk::basic_table<T>::get_cell_value(ulong row, prop_id id) const
{
    if(!prop_exists(row, id))
        throw key_not_found<prop_id>(id);

    const_column_iter column = m_columns.find(id);
    ulonglong value;

    switch(column->second.size)
    {
        case 8:
            value = read_raw_row<ulonglong>(row, column->second.offset);
            break;
        case 4:
            value = read_raw_row<ulong>(row, column->second.offset);
            break;
        case 2:
            value = read_raw_row<ushort>(row, column->second.offset);
            break;
        case 1:
            value = read_raw_row<byte>(row, column->second.offset);
            break;
        default:
            throw database_corrupt("get_cell_value: invalid cell size");
    }

    return value;
}

template<typename T>
inline size_t pstsdk::basic_table<T>::row_prop_size(ulong row, prop_id id) const
{
    heapnode_id hid = static_cast<heapnode_id>(get_cell_value(row, id));

    if(is_subnode_id(hid))
        return get_node().lookup(hid).size();
    else
        return m_prows->get_heap_ptr()->size(hid);
}

template<typename T>
inline std::vector<pstsdk::byte> pstsdk::basic_table<T>::read_cell(ulong row, prop_id id) const
{
    heapnode_id hid = static_cast<heapnode_id>(get_cell_value(row, id));
    std::vector<byte> buffer;

    if(is_subnode_id(hid))
    {
        node sub(get_node().lookup(hid));
        buffer.resize(sub.size());
        sub.read(buffer, 0);
    }
    else
    {
        buffer = m_prows->get_heap_ptr()->read(hid);
    }
    return buffer;
}

template<typename T>
inline pstsdk::hnid_stream_device pstsdk::basic_table<T>::open_cell_stream(ulong row, prop_id id)
{
    heapnode_id hid = static_cast<heapnode_id>(get_cell_value(row, id));

    if(is_subnode_id(hid))
        return get_node().lookup(hid).open_as_stream();
    else
        return m_prows->get_heap_ptr()->open_stream(hid);
}

template<typename T>
inline pstsdk::prop_type pstsdk::basic_table<T>::get_prop_type(prop_id id) const
{
    const_column_iter iter = m_columns.find(id);

    if(iter == m_columns.end())
        throw key_not_found<prop_id>(id);

    return (prop_type)iter->second.type;
}

template<typename T>
inline pstsdk::row_id pstsdk::basic_table<T>::get_row_id(ulong row) const
{
    return read_raw_row<row_id>(row, 0);
}

template<typename T>
template<typename Val>
inline Val pstsdk::basic_table<T>::read_raw_row(ulong row, ushort offset) const
{
    if(row >= size())
        throw std::out_of_range("row >= size()");

    if(m_pnode_rowarray)
    {
        ulong page_num = row / rows_per_page();
        ulong page_offset = (row % rows_per_page()) * cb_per_row();

        return m_pnode_rowarray->read<Val>(page_num, page_offset+offset);
    }
    else
    {
        Val val;
        memcpy(&val, &m_vec_rowarray[ row * cb_per_row() + offset ], sizeof(Val));
        return val;
    }
}

template<typename T>
inline std::vector<pstsdk::byte> pstsdk::basic_table<T>::read_exists_bitmap(ulong row) const
{
    std::vector<byte> exists_bitmap(cb_per_row() - exists_bitmap_start());

    if(row >= size())
        throw std::out_of_range("row >= size()");

    if(m_pnode_rowarray)
    {
        ulong page_num = row / rows_per_page();
        ulong page_offset = (row % rows_per_page()) * cb_per_row();

        m_pnode_rowarray->read(exists_bitmap, page_num, page_offset + exists_bitmap_start());
    }
    else
    {
        memcpy(&exists_bitmap[0], &m_vec_rowarray[ row * cb_per_row() + exists_bitmap_start() ], exists_bitmap.size());
    }    

    return exists_bitmap;
}

template<typename T>
inline bool pstsdk::basic_table<T>::prop_exists(ulong row, prop_id id) const
{
    const_column_iter column = m_columns.find(id);

    if(column == m_columns.end())
        return false;

    std::vector<byte> exists_map = read_exists_bitmap(row);

    return test_bit(&exists_map[0], column->second.bit_offset);
}

inline pstsdk::table::table(const node& n)
{
    m_ptable = open_table(n);
}

inline pstsdk::table::table(const table& other)
{
    m_ptable = open_table(other.m_ptable->get_node());
}

#endif
