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

#include <so_5/fwd.hpp>

#include <functional>

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
		// Use std::less for compare pointers.
		// NOTE: it's UB to compare pointers that do not belong to the same array.
		using ptr_comparator_t = std::less< const agent_t * >;

		auto p1 = a.so_priority();
		auto p2 = b.so_priority();

		// NOTE: there should be inversion -- agent with higher
		// priority must be first.
		return ( p1 > p2 || (p1 == p2 && ptr_comparator_t{}( &a, &b ) ) );
	}

} /* namespace impl */

} /* namespace so_5 */

