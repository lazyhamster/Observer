#ifndef STRING_BUILDER_INCLUDED
#define STRING_BUILDER_INCLUDED

#include <math.h>
#include <string.h>
#include "stream_base.h"

template <class Ty>
class string_builder_base : public mystream::stream_base
{
	public:
		typedef mystream::stream_base base;
		typedef Ty value_type;
		typedef Ty* value_type_ptr;
		typedef const Ty* const_value_type_ptr;
		typedef size_t size_type;
		
	private:
		value_type_ptr m_buffer;
		size_type m_bufferSize;
		size_type m_stringSize;
		size_type m_growby;
		
		bool allocate(size_type newsize)
		{
			if(allocated() < newsize) {
				value_type_ptr old=m_buffer;
				size_type oldsize=m_bufferSize;
				
				int c=(int)ceil((float)newsize / growby());
				m_bufferSize=c * growby();
				m_buffer=new value_type[m_bufferSize];
				if(m_buffer==NULL){
					setstate(failbit);
					m_buffer=old;
					m_bufferSize=oldsize;
				}
				else{
					memcpy(m_buffer, old, oldsize * sizeof(value_type));
					delete[] old;
				}
			}
			return (state() & failbit)==0;
		}
		
		void init()
		{
			m_buffer=0; m_bufferSize=m_stringSize=0; 
			base::clear(badbit);
		}
		
		size_type strlen(const wchar_t *str) { return (size_type)::wcslen(str); }
		size_type strlen(const char *str) { return (size_type)::strlen(str); }
		
	public:
		string_builder_base() { init(); growby(0); }
		string_builder_base(const_value_type_ptr str) 
		{
			init();	growby(0);
			append(str);
		}
		
		string_builder_base(const string_builder_base& str) 
		{
			init();	growby(0);
			append(str);
		}
		
		bool reserve(size_type newsize) { return allocate(newsize); }
		size_type allocated() const { return m_bufferSize; }
		size_type size() const { return m_stringSize; }
		void size(size_type newsize) { m_stringSize=(newsize > allocated() ? allocated() : newsize); }
		
		size_type growby() const { return m_growby; }
		void growby(size_type newgrow) { if(newgrow==0) newgrow=100; m_growby=newgrow; }
		
		~string_builder_base() { delete[] m_buffer; }
		
		string_builder_base& append(const_value_type_ptr str)
		{
			size_type s=strlen(str);
			size_type newsize=size() + s;
			if(allocate(newsize + 1)==false)
				return *this;
			memcpy(m_buffer + size(), str, (s + 1) * sizeof(value_type));
			size(newsize);
			base::clear(m_buffer ? goodbit : badbit);
			
			return *this;
		}
		
		string_builder_base& append(const string_builder_base& str)
		{
			size_type newsize=size() + str.size();
			if(allocate(newsize + 1)==false)
				return *this;
			memcpy(m_buffer + size(), str, (str.size() + 1) * sizeof(value_type));
			size(newsize);
			base::clear(m_buffer ? goodbit : badbit);
			
			return *this;
		}
		
		string_builder_base& append(const value_type& ch)
		{
			//if(!good()) return;
			size_type newsize=size() + 1;
			if(allocate(newsize + 1)==false) 
				return *this;
			m_buffer[size()]=ch;
			m_buffer[size() + 1]=0;
			size(newsize);
			base::clear(m_buffer ? goodbit : badbit);
			return *this;
		}
		
		value_type_ptr getbuffer() 
		{
			value_type_ptr b=m_buffer;
			init();
			return b;
		}
		
		void clear()
		{
			delete[] m_buffer;
			init();
		}
		
		const_value_type_ptr buffer() const { return m_buffer; }
		
		operator const_value_type_ptr () const { return m_buffer; }
		
		string_builder_base& operator = (const string_builder_base& other)
		{
			if(other.m_buffer==0) {
				delete[] m_buffer;
				init();
			}
			else 
				(*this)=other.m_buffer;
			
			return *this;
		}
		
		string_builder_base& operator = (const_value_type_ptr str)
		{
			size(0);
			append(str);
			base::clear(m_buffer ? goodbit : badbit);
			return *this;
		}
		
};

class string_builder : public string_builder_base<char>
{
	public:
		typedef string_builder_base<char> base;
		string_builder() : string_builder_base<char>() { }
		string_builder(const char *str) : string_builder_base<char>(str) { }
		
		string_builder& operator = (const_value_type_ptr str) { return (string_builder&)base::operator =(str); }
};

inline string_builder& operator << (string_builder& is, const char *str)
{
	is.append(str);
	return is;
}

inline string_builder& operator << (string_builder& is, char ch)
{
	is.append(ch);
	return is;
}

class wstring_builder : public string_builder_base<wchar_t>
{
	public:
		wstring_builder() : string_builder_base<wchar_t>() { }
		wstring_builder(const wchar_t *str) : string_builder_base<wchar_t>(str) { }
};

inline wstring_builder& operator << (wstring_builder& is, const wchar_t *str)
{
	is.append(str);
	return is;
}

inline wstring_builder& operator << (wstring_builder& is, wchar_t ch)
{
	is.append(ch);
	return is;
}

inline wstring_builder& operator << (wstring_builder& is, int i)
{
	is.append((wstring_builder::value_type)i);
	return is;
}

#endif // !defined(STRING_BUILDER_INCLUDED)