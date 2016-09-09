/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief A type for storing prefix of data_source name.
 */

#pragma once

#include <string>
#include <cstring>
#include <ostream>

namespace so_5
{

namespace stats
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief A type for storing prefix of data_source name.
 */
class prefix_t
	{
	public :
		//! Max length of prefix (not including 0-symbol at the end).
		static const std::size_t max_length = 47;
		//! Max size of buffer for prefix value (including 0-symbol at the end).
		static const std::size_t max_buffer_size = max_length + 1;

		//! Default constructor creates empty prefix.
		inline
		prefix_t()
			{
				m_value[ 0 ] = 0;
			}

		//! Initializing constructor.
		/*!
		 * Gets no more than max_length symbols.
		 */
		inline
		prefix_t( const char * value )
			{
				char * last = m_value + max_length;
				char * pos = m_value;
				while( *value && pos != last )
					{
						*(pos++) = *(value++);
					}
				*pos = 0;
			}

		//! Initializing constructor.
		/*!
		 * Gets no more than max_length symbols.
		 */
		inline
		prefix_t( const std::string & value )
			:	prefix_t( value.c_str() )
			{}

		//! Access to prefix value.
		inline const char *
		c_str() const
			{
				return m_value;
			}

		//! Is prefix empty?
		inline bool
		empty() const
			{
				return 0 == m_value[ 0 ];
			}

		//! Is equal?
		inline bool
		operator==( const prefix_t & o ) const
			{
				return 0 == strcmp( c_str(), o.c_str() );
			}

		//! Is not equal?
		inline bool
		operator!=( const prefix_t & o ) const
			{
				return 0 != strcmp( c_str(), o.c_str() );
			}

		//! Is less than?
		inline bool
		operator<( const prefix_t & o ) const
			{
				return 0 < strcmp( c_str(), o.c_str() );
			}

	private :
		//! Actual value.
		char m_value[ max_buffer_size ];
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief Just a helper operator.
 */
inline std::ostream &
operator<<( std::ostream & to, const prefix_t & what )
	{
		return (to << what.c_str());
	}

/*!
 * \since
 * v.5.5.4
 *
 * \brief A type for representing the suffix of data_source name.
 *
 * \note This is just a wrapper around `const char *`.
 */
class suffix_t
	{
	public :
		//! Initializing constructor.
		inline
		suffix_t( const char * value )
			:	m_value( value )
			{}

		//! Access to suffix value.
		inline const char *
		c_str() const
			{
				return m_value;
			}

		//! Compares suffixes by pointer values.
		inline bool
		operator==( const suffix_t & o ) const
			{
				return m_value == o.m_value;
			}

		//! Compares suffixes by pointer value.
		inline bool
		operator!=( const suffix_t & o ) const
			{
				return m_value != o.m_value;
			}

		//! Compares suffixes by pointer value.
		inline bool
		operator<( const suffix_t & o ) const
			{
				return m_value < o.m_value;
			}

	private :
		//! Actual value.
		const char * m_value;
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief Just a helper operator.
 */
inline std::ostream &
operator<<( std::ostream & to, const suffix_t & what )
	{
		return (to << what.c_str());
	}

} /* namespace stats */

namespace rt
{

namespace stats
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::stats::prefix_t
 * instead.
 */
using prefix_t = so_5::stats::prefix_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::stats::suffix_t
 * instead.
 */
using suffix_t = so_5::stats::suffix_t;

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */

