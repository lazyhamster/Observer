#ifndef NullStream_h__
#define NullStream_h__

// Special stream that discards all incoming data
// but calculates amount of bytes consumed

#include <streambuf>
#include <ostream>

template <class cT, class traits = std::char_traits<cT> >
class basic_nullbuf: public std::basic_streambuf<cT, traits>
{
	virtual typename traits::int_type overflow(typename traits::int_type c)
	{
		m_nPos++;
		return traits::not_eof(c); // indicate success
	}

	virtual pos_type __CLR_OR_THIS_CALL seekoff(
		off_type _Off,
		std::ios_base::seekdir _Way,
		std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
	{
		switch (_Way)
		{
			case std::ios_base::beg:
				m_nPos = _Off;
				break;
			case std::ios_base::cur:
				m_nPos += _Off;
				break;
			case std::ios_base::end:
				return (streampos(_BADOFF));
		}
		if (m_nPos < 0) m_nPos = 0;

		return m_nPos;
	}

private:
	off_type m_nPos;
};

template <class cT, class traits = std::char_traits<cT> >
class basic_onullstream: public std::basic_ostream<cT, traits>
{
public:
	basic_onullstream():
	  std::basic_ios<cT, traits>(&m_sbuf),
		  std::basic_ostream<cT, traits>(&m_sbuf)
	  {
		  init(&m_sbuf);
	  }

private:
	basic_nullbuf<cT, traits> m_sbuf;
};

typedef basic_onullstream<char> onullstream;
typedef basic_onullstream<wchar_t> wonullstream;

#endif // NullStream_h__