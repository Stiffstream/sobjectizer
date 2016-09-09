/*
	SObjectizer 5.
*/

/*!
 * \since
 * v.5.5.8
 *
 * \file
 * \brief A special comparison for agent pointers.
 */

#pragma once

#include <so_5/rt/h/agent.hpp>

namespace so_5 {

namespace impl {

/*!
 * \since
 * v.5.5.8
 *
 * \file
 * \brief A special comparison for agent pointers with respect to
 * agent's priorities.
 */
inline bool
special_agent_ptr_compare( const agent_t & a, const agent_t & b )
	{
		auto p1 = a.so_priority();
		auto p2 = b.so_priority();

		// NOTE: there should be inversion -- agent with higher
		// priority must be first.
		return ( p1 > p2 || (p1 == p2 && &a < &b) );
	}

} /* namespace impl */

} /* namespace so_5 */

