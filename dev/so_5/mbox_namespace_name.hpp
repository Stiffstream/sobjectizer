/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A mbox_namespace_name_t class definition.
	\since v.5.8.0
*/

#pragma once

#include <string_view>

#include <so_5/ret_code.hpp>
#include <so_5/exception.hpp>

namespace so_5
{

//
// mbox_namespace_name_t
//

//! A class for the name of mbox_namespace.
/*!
 * Checks value in the constructor. Throws an exception if the name is empty.
 *
 * \note
 * This class can be seen as a strong typedef for std::string_view.
 * It doesn't create a copy of the name, just holds a "view" to the name.
 *
 * \since v.5.8.0
 */
class mbox_namespace_name_t
	{
		//! The name.
		std::string_view m_name;

	public:
		/*!
		 * There is no way to create an empty name.
		 */
		mbox_namespace_name_t() = delete;

		//! Constructor with check for the empty value.
		explicit mbox_namespace_name_t( std::string_view name )
			:	m_name( std::move(name) )
			{
				if( m_name.empty() )
					SO_5_THROW_EXCEPTION( rc_empty_name, "empty mbox_namespace name" );
			}

		//! Get the value.
		[[nodiscard]]
		std::string_view
		query_name() const
			noexcept( noexcept( std::string_view{m_name} ) )
			{
				return m_name;
			}
	};

} /* namespace so_5 */

