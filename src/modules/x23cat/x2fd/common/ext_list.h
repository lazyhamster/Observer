/****************************************
 *
 * List class - last modified 3.5. 2005
 *
 ****************************************/

#ifndef EXT_LIST_INCLUDED
#define EXT_LIST_INCLUDED

#include <stddef.h>

namespace ext
{

template <class T> class list
{
	public:
		typedef T value_type;
		typedef size_t size_type;
		typedef T& reference;
		typedef const T& const_reference;

	private:
		class node
		{
			public:
				node *prev, *next;
				value_type val;
		};

	public:
		class const_iterator
		{
			private:
				node *_mynode;

			public:
				node* mynode() const { return _mynode; }

				const_iterator() { _mynode=0; }
				const_iterator(node* n) { _mynode=n; }

				const_reference operator *() const { return _mynode->val; }
				const_reference operator ->() const { return _mynode->val; }

				bool operator ==(const const_iterator &right) const { return _mynode==right.mynode(); }
				bool operator !=(const const_iterator &right) const { return _mynode!=right.mynode(); }

				const_iterator operator++(int) { _mynode=_mynode->next; return const_iterator(_mynode->prev); }
				const_iterator& operator++() { _mynode=_mynode->next; return *this; }

				const_iterator operator--(int) { _mynode=_mynode->prev; return const_iterator(_mynode->next); }
				const_iterator& operator--() { _mynode=_mynode->prev; return *this; }

				const_iterator operator+(int offset)
				{
					const_iterator it(mynode());
					for(int i=0; i < offset; ++i, ++it)
						;
					return it;
				}

				const_iterator operator-(int offset)
				{
					const_iterator it(mynode());
					for(int i=offset; i > 0; --i, --it)
						;
					return it;
				}
		};

		class iterator : public const_iterator
		{
			public:
				typedef const_iterator base;

				iterator() { }
				iterator(node* n) : const_iterator(n) { }

				reference operator *() { return base::mynode()->val; }
				reference operator ->() { return base::mynode()->val; }

				bool operator ==(const iterator &right) const { return base::mynode()==right.base::mynode(); }
				bool operator !=(const iterator &right) const { return base::mynode()!=right.base::mynode(); }

				iterator operator++(int) { iterator tmp=*this; ++*this; return tmp; }
				iterator& operator++() { ++(*(const_iterator*)this); return *this; }

				iterator operator--(int) { iterator tmp=*this; --*this; return tmp; }
				iterator& operator--() { --(*(const_iterator*)this); return *this; }

				iterator operator+(int offset)
				{
					iterator it(base::mynode());
					for(int i=0; i < offset; ++i, ++it);
					return it;
				}

				iterator operator-(int offset)
				{
					iterator it(base::mynode());
					for(int i=offset; i > 0; --i, --it)
						;
					return it;
				}

		};

		class const_reverse_iterator
		{
			private:
				node *_mynode;

			public:
				node* mynode() const { return _mynode; }

				const_reverse_iterator() { _mynode=0; }
				const_reverse_iterator(node* n) { _mynode=n; }

				const_reference operator *() const { return _mynode->val; }
				const_reference operator ->() const { return _mynode->val; }

				bool operator ==(const const_reverse_iterator &right) const { return _mynode==right.mynode(); }
				bool operator !=(const const_reverse_iterator &right) const { return _mynode!=right.mynode(); }

				const_reverse_iterator operator++(int) { _mynode=_mynode->prev; return const_reverse_iterator(_mynode->next); }
				const_reverse_iterator& operator++() { _mynode=_mynode->prev; return *this; }

				const_reverse_iterator operator--(int) { _mynode=_mynode->next; return const_reverse_iterator(_mynode->prev); }
				const_reverse_iterator& operator--() { _mynode=_mynode->next; return *this; }

				const_reverse_iterator operator+(int offset)
				{
					const_reverse_iterator it(mynode());
					for(int i=0; i < offset; ++i, ++it)
						;;
					return it;
				}

				const_reverse_iterator operator-(int offset)
				{
					const_reverse_iterator it(mynode());
					for(int i=offset; i > 0; --i, --it)
						;;
					return it;
				}
		};

		class reverse_iterator : public const_reverse_iterator
		{
			public:
				typedef const_reverse_iterator base;

				reverse_iterator() { }
				reverse_iterator(node* n) : const_reverse_iterator(n) { }

				reference operator *() { return base::mynode()->val; }
				reference operator ->() { return base::mynode()->val; }

				bool operator ==(const reverse_iterator &right) const { return base::mynode()==right.base::mynode(); }
				bool operator !=(const reverse_iterator &right) const { return base::mynode()!=right.base::mynode(); }

				reverse_iterator operator++(int) { reverse_iterator tmp=*this; ++*this; return tmp; }
				reverse_iterator& operator++() { ++(*(const_reverse_iterator*)this); return *this; }

				reverse_iterator operator--(int) { reverse_iterator tmp=*this; --*this; return tmp; }
				reverse_iterator& operator--() { --(*(const_reverse_iterator*)this); return *this; }

				reverse_iterator operator+(int offset)
				{
					reverse_iterator it(base::mynode());
					for(int i=0; i < offset; ++i, ++it)
						;;
					return it;
				}

				iterator operator-(int offset)
				{
					reverse_iterator it(base::mynode());
					for(int i=offset; i > 0; --i, --it)
						;;
					return it;
				}
		};

	private:
		node *myhead;
		size_type count;

		static node* nextnode(node *n) { return n->next; }
		static node* prevnode(node *n) { return n->prev; }
		static reference nodeval(node *n) { return n->val; }

		void _insert(iterator where, const_reference val)
		{
			node *newnode=new node(), *nodeptr=where.mynode();
			newnode->prev=nodeptr->prev;
			newnode->next=nodeptr;
			newnode->prev->next=newnode;
			nodeptr->prev=newnode;

			newnode->val=val;
			count++;
		}

	public:
		list()
		{
			myhead=new node();
			myhead->next=myhead;
			myhead->prev=myhead;
			count=0;
		}

		list(const list& other)
		{
			myhead=new node();
			myhead->next=myhead;
			myhead->prev=myhead;
			count=0;

			insert(end(), other.begin(), other.end());
		}

		virtual ~list()
		{
			clear();
			delete myhead;
		}

		void clear()
		{
			node *n, *d;
			n=d=nextnode(myhead);
			for( ; n!=myhead; d=n){
				n=nextnode(n);
				delete d;
			}
			myhead->next=myhead;
			myhead->prev=myhead;
			count=0;
		}

		size_type size() const { return count; }

		iterator begin() { return iterator(nextnode(myhead)); }
		iterator end() { return iterator(myhead); }

		const_iterator begin() const { return iterator(nextnode(myhead)); }
		const_iterator end() const { return iterator(myhead); }

		reverse_iterator rbegin() { return reverse_iterator(prevnode(myhead)); }
		reverse_iterator rend() { return reverse_iterator(myhead); }

		const_reverse_iterator rbegin() const { return reverse_iterator(prevnode(myhead)); }
		const_reverse_iterator rend() const { return reverse_iterator(myhead); }

		iterator insert(iterator where, const_reference val)
		{
			_insert(where, val);
			return --where;
		}

		void insert(iterator where, const_iterator first, const_iterator last)
		{
			for(; first!=last; ++first){
				_insert(where, *first);
			}
		}

		iterator erase(iterator where)
		{
			node *delnode=(where++).mynode();
			if(delnode!=myhead){
				delnode->prev->next=delnode->next;
				delnode->next->prev=delnode->prev;
				delete delnode;
				count--;
			}
			return where;
		}

		iterator erase(iterator first, iterator last)
		{
			if(first==begin() && last==end())
				clear();
			else{
				while(first!=last)
					first=erase(first);
			}
			return last;
		}

		iterator push_front(const_reference val)
		{
			_insert(begin(), val);
			return begin();
		}

		iterator push_back(const_reference val)
		{
			_insert(end(), val);
			return --end();
		}

		bool empty() const { return size()==0; }

		void pop_front() { erase(begin()); }
		void pop_back() { erase(--end()); }

		const_reference front() const { return nodeval(myhead); }
		reference front() { return nodeval(myhead->next); }

		const_reference back() const { return nodeval(myhead->prev); }
		reference back() { return nodeval(myhead->prev); }

		iterator find(const_reference val)
		{
			for(iterator it=begin(); it!=end(); ++it){
				if(*it==val)
					return it;
			}
			return end();
		}

		void remove(const_reference val)
		{
			iterator old;
			for(iterator &it=begin(); it!=end(); ++it){
				if(*it==val){
					old=it - 1;
					_erase(it);
					it=old;
				}
			}
		}
};

}	// namespace ext

#endif // !defined(EXT_LIST_DEFINED)