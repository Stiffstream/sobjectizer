/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A nonempty_name class definition.
*/

#pragma once

#include <string>

#include <so_5/ret_code.hpp>
#include <so_5/exception.hpp>

namespace so_5
{

//
// nonempty_name_t
//

//! A class for the name which cannot be empty.
/*!
 * Checks value in the constructor. Throws an exception if the name is empty.
*/
class nonempty_name_t
{
		// Note: clang-3.9 requires this on Windows platform.
		nonempty_name_t( const nonempty_name_t & ) = delete;
		nonempty_name_t & operator=( const nonempty_name_t & ) = delete;

	private:
		//! Value.
		std::string m_nonempty_name;

	public:
		//! Constructor with check for the empty value.
		nonempty_name_t( const char * name )
			:	nonempty_name_t( std::string(name) )
		{}

		//! Constructor with check for the empty value.
		nonempty_name_t( std::string name )
			:	m_nonempty_name( std::move(name) )
		{
			if( m_nonempty_name.empty() )
				SO_5_THROW_EXCEPTION( rc_empty_name, "empty string as argument" );
		}

		nonempty_name_t( nonempty_name_t && o ) noexcept
			:	m_nonempty_name( std::move(o.m_nonempty_name) )
		{}

		friend void
		swap( nonempty_name_t & a, nonempty_name_t & b ) noexcept
		{
			swap( a.m_nonempty_name, b.m_nonempty_name );
		}

		nonempty_name_t &
		operator=( nonempty_name_t && o ) noexcept
		{
			nonempty_name_t tmp( std::move(o) );
			swap( *this, o );
			return *this;
		}

		//! Get the value.
		[[nodiscard]]
		inline const std::string &
		query_name() const noexcept
		{
			return m_nonempty_name;
		}

		/*!
		 * \brief Get the value away from the object.
		 *
		 * \attention
		 * After calling that method m_nonempty_name will become empty!
		 *
		 * \since
		 * v.5.5.19
		 */
		[[nodiscard]]
		std::string
		giveout_value()
			noexcept( noexcept( std::string{ std::move(m_nonempty_name) } ) )
		{
			return std::move(m_nonempty_name);
		}
};

} /* namespace so_5 */

