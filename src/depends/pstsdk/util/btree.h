//! \file
//! \brief Generic BTree implementation
//! \author Terry Mahaffey
//!
//! BTrees show up three times in the PST file format - the NBT/BBT, in
//! sub_blocks, and in the BTH. It is useful to have some generic BTree
//! hierarchy setup to centralize some common BTree logic such as iteration,
//! lookups, etc. This file lays out that structure. Specific BTree 
//! implementations will inherit and minimic this class hierarchy, and only
//! provide implementations of a few virtual functions.
//! \ingroup util

//! \defgroup btree BTree
//! \ingroup util

#ifndef PSTSDK_UTIL_BTREE_H
#define PSTSDK_UTIL_BTREE_H

#include <iterator>
#include <vector>
#include <boost/iterator/iterator_facade.hpp>

#include "pstsdk/util/primitives.h"
#include "pstsdk/util/errors.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4250)
#endif

namespace pstsdk
{

template<typename K, typename V>
struct btree_iter_impl;

template<typename K, typename V>
class const_btree_node_iter;

template<typename K, typename V>
class btree_node_nonleaf;

//! \brief A BTree Node
//!
//! The fundamental type in the BTree system is the btree_node. It provides
//! the generic interface which is refined by the other classes.
//! \param K The key type. Must be LessThan comparable.
//! \param V The value type
//! \ingroup btree
template<typename K, typename V>
class btree_node
{
public:
    typedef const_btree_node_iter<K,V> const_iterator;

    virtual ~btree_node() { }

    //! \brief Looks up the associated value for a given key
    //!
    //! This will defer to child btree_nodes as appropriate
    //! \throw key_not_found<K> if the requested key is not in this btree
    //! \param[in] key The key to lookup
    //! \returns The associated value
    virtual const V& lookup(const K& key) const = 0;
    
    //! \brief Returns the key at the specified position
    //!
    //! This is specific to this btree_node, not the entire tree
    //! \param[in] pos The position to retrieve the key for
    //! \returns The key at the requested position
    virtual const K& get_key(uint pos) const = 0;

    //! \brief Returns the number of entries in this btree_node
    //!
    //! This is specific to this btree_node, not the entire tree
    //! \returns The number of keys in this btree_node
    virtual uint num_values() const = 0;

    //! \brief Returns a STL style iterator positioned at the first entry
    //!
    //! Supports iteration over the value members of this btree. This method
    //! returns an iterator positioned on the first element of the tree. One
    //! can then 
    //! \returns An iterator over the values in this btree
    const_iterator begin() const
        { return const_iterator(this, false); }

    //! \brief Returns a STL style iterator positioned at the "end" entry
    //!
    //! Supports iteration over the value members of this btree. This method
    //! returns an iterator positioned past the last element of this btree.
    //! \returns An iterator over the values in this btree
    const_iterator end() const
        { return const_iterator(this, true); }

    //! \brief Performs a binary search over the keys of this btree_node
    //! \param[in] key The key to lookup
    //! \returns The position of the key, or of the entry which would contain it
    int binary_search(const K& key) const;

protected:

    // iter support
    friend class const_btree_node_iter<K,V>;
    friend class btree_node_nonleaf<K,V>;
    //! \brief Positions the iterator at the first element on this tree
    //! \param[in,out] iter Iterator state class
    virtual void first(btree_iter_impl<K,V>& iter) const = 0;
    //! \brief Positions the iterator at the "end" element
    //! \param[in,out] iter Iterator state class
    virtual void last(btree_iter_impl<K,V>& iter) const = 0;
    //! \brief Moves the iterator to the next element
    //! \param[in,out] iter Iterator state class
    virtual void next(btree_iter_impl<K,V>& iter) const = 0;
    //! \brief Moves the iterator to the previous element
    //! \param[in,out] iter Iterator state class
    virtual void prev(btree_iter_impl<K,V>& iter) const = 0;
};

//! \brief Represents a leaf node in a BTree structure
//!
//! Classes which model a leaf of a BTree structure inherit from this.
//! They have a total of three simple virtual functions to implement:
//! - num_values, from btree_node
//! - get_key(uint pos), from btree_node
//! - get_value(uint pos)
//! \param K The key type. Must be LessThan comparable.
//! \param V The value type
//! \ingroup btree
template<typename K, typename V>
class btree_node_leaf : public virtual btree_node<K,V>
{
public:
    virtual ~btree_node_leaf() { }

    //! \brief Looks up the associated value for a given key
    //! \throw key_not_found<K> if the requested key is not in this btree
    //! \param[in] key The key to lookup
    //! \returns The associated value
    const V& lookup(const K& key) const;

    //! \brief Returns the value at the associated position on this leaf node
    //! \param[in] pos The position to retrieve the value for
    //! \returns The value at the requested position
    virtual const V& get_value(uint pos) const = 0;

protected:

    // iter support
    friend class const_btree_node_iter<K,V>;
    void first(btree_iter_impl<K,V>& iter) const
        { iter.m_leaf = const_cast<btree_node_leaf<K,V>* >(this); iter.m_leaf_pos = 0; }
    void last(btree_iter_impl<K,V>& iter) const
        { iter.m_leaf = const_cast<btree_node_leaf<K,V>* >(this); iter.m_leaf_pos = this->num_values()-1; }
    void next(btree_iter_impl<K,V>& iter) const;
    void prev(btree_iter_impl<K,V>& iter) const;
};

//! \brief Represents a non-leaf node in a BTree structure
//!
//! Classes which model a non-leaf of a BTree structure inherit from this.
//! They have a total of four simple virtual functions to implement:
//! - num_values, from btree_node
//! - get_key(uint pos), from btree_node
//! - get_child(uint pos)
//! - get_child(uint pos) const, const version of above
//! \param K The key type. Must be LessThan comparable.
//! \param V The value type
//! \ingroup btree
template<typename K, typename V>
class btree_node_nonleaf : public virtual btree_node<K,V>
{
public:
    virtual ~btree_node_nonleaf() { }

    //! \copydoc btree_node::lookup
    const V& lookup(const K& key) const;

protected:
    //! \brief Returns the child btree_node at the requested location
    //! \param[in] i The position at which to get the child
    //! \returns a non-owning pointer of the child btree_node
    virtual btree_node<K,V>* get_child(uint i) = 0;
    //! \brief Returns the child btree_node at the requested location
    //! \param[in] i The position at which to get the child
    //! \returns a non-owning pointer of the child btree_node
    virtual const btree_node<K,V>* get_child(uint i) const = 0;

    // iter support
    friend class const_btree_node_iter<K,V>;
    friend class btree_node_leaf<K,V>;
    void first(btree_iter_impl<K,V>& iter) const;
    void last(btree_iter_impl<K,V>& iter) const;
    void next(btree_iter_impl<K,V>& iter) const;
    void prev(btree_iter_impl<K,V>& iter) const;
};

//! \brief BTree iterator helper class
//!
//! This is a utility struct, the details of which are known to both the iterator
//! type itself and the various node classes. The iterator class defers to the
//! btree_node classes for the actual iteration logic. It does this by calling
//! into them (private methods, via friendship) and passing in this object. 
//! \param K key type
//! \param V value type
//! \ingroup btree
template<typename K, typename V>
struct btree_iter_impl
{
    btree_node_leaf<K,V>* m_leaf;   //!< The current leaf btree node this iterator is pointing to
    uint m_leaf_pos;                //!< The current position on that leaf

    std::vector<std::pair<btree_node_nonleaf<K,V>*, uint> > m_path; //!< The "path" to this leaf, starting at the root of the btree
    typedef typename std::vector<std::pair<btree_node_nonleaf<K,V>*, uint> >::iterator path_iter;
};

//! \brief The actual iterator type used by the btree_node class hierarchy
//!
//! We use the boost iterator library here to implement our iterator.
//! All of the actual iteration logic is contained in the first, last, next,
//! and prev member functions of the btree_node classes. They know how to 
//! modify the btree_iter_impl state class to move between various btree
//! pages. The iterator class itself therefore is very light weight.
//! \param K key type
//! \param V value type
//! \ingroup btree
template<typename K, typename V>
class const_btree_node_iter : public boost::iterator_facade<const_btree_node_iter<K,V>, const V, boost::bidirectional_traversal_tag>
{
public:
    //! \brief Default constructor
    const_btree_node_iter();

    //! \brief Constructs an iterator from a root node
    //! \param[in] root The root of the BTree to iterator over
    //! \param[in] last True if this is a begin iterator, false for an end iterator
    const_btree_node_iter(const btree_node<K,V>* root, bool last);

private:
    friend class boost::iterator_core_access;

    //! \brief Moves the iterator to the next element
    void increment() { m_impl.m_leaf->next(m_impl); }

    //! \brief Compares two iterators for equality
    //! \param[in] other The iterator to compare against
    bool equal(const const_btree_node_iter& other) const 
        { return ((m_impl.m_leaf == other.m_impl.m_leaf) && (m_impl.m_leaf_pos == other.m_impl.m_leaf_pos)); }

    //! \brief Returns the value this iterator points at
    //! \returns The value this iterator is pointing at
    const V& dereference() const
        { return m_impl.m_leaf->get_value(m_impl.m_leaf_pos); }

    //! \brief Moves the iterator to the previous element
    void decrement() { m_impl.m_leaf->prev(m_impl); }

    mutable btree_iter_impl<K,V> m_impl;    //!< Iterator state
};

} // end namespace

template<typename K, typename V>
int pstsdk::btree_node<K,V>::binary_search(const K& k) const
{
    uint end = num_values();
    uint start = 0;
    uint mid = (start + end) / 2;

    while(mid < end)
    {
        if(get_key(mid) < k)
        {
            start = mid + 1;
        }
        else if(get_key(mid) == k)
        {
            return mid; 
        }
        else
        {
            end = mid;
        }

        mid = (start + end) / 2;
    }

    return mid - 1;
}

template<typename K, typename V>
const V& pstsdk::btree_node_leaf<K,V>::lookup(const K& k) const
{
    int location = this->binary_search(k);

    if(location == -1)
        throw key_not_found<K>(k);
    
    if(this->get_key(location) != k)
        throw key_not_found<K>(k);

    return get_value(location);
}
    
template<typename K, typename V>
void pstsdk::btree_node_leaf<K,V>::next(btree_iter_impl<K,V>& iter) const
{
    if(++iter.m_leaf_pos == this->num_values())
    {
        if(iter.m_path.size() > 0)
        {
            for(typename btree_iter_impl<K,V>::path_iter piter = iter.m_path.begin();
                piter != iter.m_path.end(); 
                ++piter)
            {
                if((*piter).second + 1 < (*piter).first->num_values())
                {
                    // we're done with this leaf
                    iter.m_leaf = NULL;
                    
                    iter.m_path.back().first->next(iter);
                    break;
                }
            }
        }
    }
}

template<typename K, typename V>
void pstsdk::btree_node_leaf<K,V>::prev(btree_iter_impl<K,V>& iter) const
{
    if(iter.m_leaf_pos == 0)
    {
        if(iter.m_path.size() > 0)
        {
            for(typename btree_iter_impl<K,V>::path_iter piter = iter.m_path.begin();
                piter != iter.m_path.end();
                ++piter)
            {
                // we're done with this leaf
                iter.m_leaf = NULL;
                
                if((*piter).second != 0 && (*piter).first->num_values() > 1)
                {
                    iter.m_path.back().first->prev(iter);
                    break;
                }
            }
        }
    }
    else
    {
        --iter.m_leaf_pos;
    }
}

template<typename K, typename V>
const V& pstsdk::btree_node_nonleaf<K,V>::lookup(const K& k) const
{
    int location = this->binary_search(k);

    if(location == -1)
        throw key_not_found<K>(k);

    return get_child(location)->lookup(k);
}

template<typename K, typename V>
void pstsdk::btree_node_nonleaf<K,V>::first(btree_iter_impl<K,V>& iter) const
{
    iter.m_path.push_back(std::make_pair(const_cast<btree_node_nonleaf<K,V>*>(this), 0));
    get_child(0)->first(iter);
}

template<typename K, typename V>
void pstsdk::btree_node_nonleaf<K,V>::last(btree_iter_impl<K,V>& iter) const
{
    iter.m_path.push_back(std::make_pair(const_cast<btree_node_nonleaf<K,V>*>(this), this->num_values()-1));
    get_child(this->num_values()-1)->last(iter);
}

template<typename K, typename V>
void pstsdk::btree_node_nonleaf<K,V>::next(btree_iter_impl<K,V>& iter) const
{
    std::pair<btree_node_nonleaf<K,V>*, uint>& me = iter.m_path.back();

    if(++me.second == this->num_values())
    {
        // we're done, pop us off the path and call up
        if(iter.m_path.size() > 1)
        {
            iter.m_path.pop_back();
            iter.m_path.back().first->next(iter);
        }
    }
    else
    {
        // call into the next leaf
        get_child(me.second)->first(iter);
    }
}

template<typename K, typename V>
void pstsdk::btree_node_nonleaf<K,V>::prev(btree_iter_impl<K,V>& iter) const
{
    std::pair<btree_node_nonleaf<K,V>*, uint>& me = iter.m_path.back();

    if(me.second == 0)
    {
        // we're done, pop us off the path and call up
        if(iter.m_path.size() > 1)
        {
            iter.m_path.pop_back();
            iter.m_path.back().first->prev(iter);
        }
    }
    else
    {
        // call into the next child
        get_child(--me.second)->last(iter);
    }
}

template<typename K, typename V>
pstsdk::const_btree_node_iter<K,V>::const_btree_node_iter()
{
    m_impl.m_leaf_pos = 0;
    m_impl.m_leaf = NULL;
}

template<typename K, typename V>
pstsdk::const_btree_node_iter<K,V>::const_btree_node_iter(const btree_node<K,V>* root, bool last)
{
    if(last)
    {
        root->last(m_impl);
        ++m_impl.m_leaf_pos;
    }
    else
    {
        root->first(m_impl);
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
