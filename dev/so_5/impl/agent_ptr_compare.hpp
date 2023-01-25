/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief A special comparison for agent pointers.
 *
 * \since v.5.5.8
 */

#pragma once

#include <so_5/impl/message_sink_ptr_compare.hpp>

namespace so_5 {

namespace impl {

/*!
 * \brief A special comparison for agent pointers with respect to
 * agent's priorities.
 *
 * \retval true if \a a is less than \a b.
 *
 * \since v.5.5.8
 */
[[nodiscard]]
inline bool
special_agent_ptr_compare( const agent_t & a, const agent_t & b ) noexcept
	{
		return special_message_sink_ptr_compare( a, b );
	}

} /* namespace impl */

} /* namespace so_5 */

