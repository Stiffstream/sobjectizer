/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A nonempty_name class definition.
*/

#pragma once

#include <string>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/exception.hpp>

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

		nonempty_name_t( nonempty_name_t && o )
			:	m_nonempty_name( std::move(o.m_nonempty_name) )
		{}

		friend void swap( nonempty_name_t & a, nonempty_name_t & b )
		{
			a.m_nonempty_name.swap( b.m_nonempty_name );
		}

		nonempty_name_t & operator=( nonempty_name_t && o )
		{
			nonempty_name_t tmp( std::move(o) );
			swap( *this, o );
			return *this;
		}

		//! Get the value.
		inline const std::string &
		query_name() const
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
		std::string
		giveout_value()
		{
			std::string r;
			r.swap( m_nonempty_name );
			return r;
		}

	private:
		//! Value.
		std::string m_nonempty_name;
};

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::nonempty_name_t
 * instead.
 */
using nonempty_name_t = so_5::nonempty_name_t;

} /* namespace rt */

} /* namespace so_5 */

