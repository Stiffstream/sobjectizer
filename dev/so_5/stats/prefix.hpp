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
#include <string_view>
#include <cstring>
#include <ostream>

namespace so_5
{

namespace stats
{

/*!
 * \brief A type for storing prefix of data_source name.
 *
 * \since v.5.5.4
 */
class prefix_t
	{
	public :
		//! Max length of prefix (not including 0-symbol at the end).
		static constexpr const std::size_t max_length = 47;
		//! Max size of buffer for prefix value (including 0-symbol at the end).
		static constexpr const std::size_t max_buffer_size = max_length + 1;

		//! Default constructor creates empty prefix.
		constexpr prefix_t() noexcept
			:	m_value{ 0 }
			{}

// clang++ complaints about unsafe pointer arithmetics.
// Attempt to use strncpy leads to warnings in VC++.
#if defined(__clang__) && (__clang_major__ >= 16)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
		//! Initializing constructor.
		/*!
		 * Gets no more than max_length symbols.
		 */
		constexpr prefix_t( const char * value ) noexcept
			:	m_value{}
			{
				char * last = m_value + max_length;
				char * pos = m_value;
				while( *value && pos != last )
					{
						*(pos++) = *(value++);
					}
				*pos = 0;
			}
#if defined(__clang__) && (__clang_major__ >= 16)
#pragma clang diagnostic pop
#endif

		//! Initializing constructor.
		/*!
		 * Gets no more than max_length symbols.
		 */
		prefix_t( const std::string & value ) noexcept( noexcept(value.c_str()) )
			:	prefix_t( value.c_str() )
			{}

		//! Access to prefix value.
		[[nodiscard]]
		constexpr const char *
		c_str() const noexcept
			{
				return m_value;
			}

		//! Access to prefix value as string_view.
		[[nodiscard]]
		constexpr std::string_view
		as_string_view() const noexcept( noexcept(std::string_view{m_value}) )
			{
				return { m_value };
			}

		//! Is prefix empty?
		[[nodiscard]]
		constexpr bool
		empty() const noexcept
			{
				return 0 == m_value[ 0 ];
			}

		//! Is equal?
		[[nodiscard]]
		bool
		operator==( const prefix_t & o ) const noexcept
			{
				return 0 == strcmp( c_str(), o.c_str() );
			}

		//! Is not equal?
		[[nodiscard]]
		bool
		operator!=( const prefix_t & o ) const noexcept
			{
				return 0 != strcmp( c_str(), o.c_str() );
			}

		//! Is less than?
		[[nodiscard]]
		bool
		operator<( const prefix_t & o ) const noexcept
			{
				return 0 < strcmp( c_str(), o.c_str() );
			}

	private :
		//! Actual value.
		char m_value[ max_buffer_size ];
	};

/*!
 * \brief Just a helper operator.
 *
 * \since v.5.5.4
 */
inline std::ostream &
operator<<( std::ostream & to, const prefix_t & what )
	{
		return (to << what.c_str());
	}

/*!
 * \brief A type for representing the suffix of data_source name.
 *
 * \note This is just a wrapper around `const char *`.
 *
 * \attention
 * It's assumed that suffix_t holds a pointer to a string in static memory
 * (it's a pointer to a string literal).
 *
 * \since v.5.5.4
 */
class suffix_t
	{
	public :
		//! Initializing constructor.
		constexpr suffix_t(
			//! Value. It's expected to be a pointer to a null-terminated string.
			const char * value ) noexcept
			:	m_value( value )
			{}

		//! Access to suffix value.
		[[nodiscard]]
		constexpr const char *
		c_str() const noexcept
			{
				return m_value;
			}

		//! Access to prefix value as string_view.
		[[nodiscard]]
		constexpr std::string_view
		as_string_view() const noexcept( noexcept(std::string_view{m_value}) )
			{
				return { m_value };
			}

		//! Compares suffixes by pointer values.
		[[nodiscard]]
		constexpr bool
		operator==( const suffix_t & o ) const noexcept
			{
				return m_value == o.m_value;
			}

		//! Compares suffixes by pointer value.
		[[nodiscard]]
		constexpr bool
		operator!=( const suffix_t & o ) const noexcept
			{
				return m_value != o.m_value;
			}

		//! Compares suffixes by pointer value.
		[[nodiscard]]
		constexpr bool
		operator<( const suffix_t & o ) const noexcept
			{
				return m_value < o.m_value;
			}

	private :
		//! Actual value.
		const char * m_value;
	};

/*!
 * \brief Just a helper operator.
 *
 * \since v.5.5.4
 */
inline std::ostream &
operator<<( std::ostream & to, const suffix_t & what )
	{
		return (to << what.c_str());
	}

} /* namespace stats */

} /* namespace so_5 */

