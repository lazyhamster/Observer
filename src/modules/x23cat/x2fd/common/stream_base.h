#ifndef STREAM_BASE_INCLUDED
#define STREAM_BASE_INCLUDED

namespace mystream {

class stream_base
{
	public:
		enum state
		{
			goodbit=0,
			failbit=1,
			eofbit=2,
			badbit=4
		};

		enum seek_type{
			seek_begin,
			seek_end,
			seek_current
		};

		struct pos_type;

		struct off_type
		{
			int offset;
			off_type() { offset=0; }
			off_type(const off_type& other) { offset=other.offset; }
			explicit off_type(const pos_type& other) { offset=(int)other.pos; }
			off_type(size_t s) { offset=(int)s; }

			operator long() { return (long) offset; }
			off_type operator -(const off_type& other) const { return (off_type)(offset - other.offset); }
			off_type operator -(int other) const { return (off_type)(offset - other); }
			off_type operator +(const off_type& other) const { return (off_type)(offset + other.offset); }
			off_type operator +(int i) const { return (off_type)(offset + i); }
			off_type operator +=(const off_type& other) { return offset+=other.offset; }
			bool operator ==(const off_type& other) const { return offset==other.offset; }
		};

		typedef off_type offset_type;

		struct pos_type
		{
			size_t pos;
			pos_type() { pos=0; }
			pos_type(const pos_type& other) { pos=other.pos; }
			pos_type(size_t s) { pos=s; }
			offset_type operator -(const pos_type& other) const { return (offset_type)(pos - other.pos); }
			offset_type operator +(const pos_type& other) const { return (offset_type)(pos + other.pos); }
			bool operator ==(const pos_type& other) const { return pos==other.pos; }
			bool operator ==(int i) const { return pos == (size_t)i; }
			
			operator size_t () const { return (size_t)pos; }
		};
		
		typedef size_t size_type;

	private:
		int m_state;
		int m_flags;

	public:
		stream_base() { m_flags=0; clear(goodbit); }
		virtual ~stream_base() { }

		void clear(state s) { m_state=s; }

		int setstate(state s)
		{
			int old=m_state;
			m_state|=s;
			return old;
		}

		int rdstate() const { return m_state; }

		bool good() const { return rdstate() == goodbit; }
		bool bad() const { return (rdstate() & badbit) > 0; }
		bool fail() const { return ((rdstate() & (failbit | badbit))!=0); }
		bool eof() const { return (rdstate() & eofbit) > 0; }

		int flags() const	{	return m_flags;	}

		int flags(int newflags)
		{
			int old=m_flags;
			m_flags = newflags;
			return old;
		}

		int setf(int newflags)
		{
			int old=m_flags;
			m_flags|=newflags;
			return old;
		}

		void unsetf(int mask)	{	m_flags&=~mask;	}
};

}

#endif // !defined(STREAM_BASE_INCLUDED)