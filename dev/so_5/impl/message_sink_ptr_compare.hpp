/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief A special comparison for pointers to message sinks.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/message_sink.hpp>

namespace so_5 {

namespace impl {

/*!
 * \brief A special comparison for pointers to message sinks with respect to
 * priorities.
 *
 * \retval true if \a a is less than \a b.
 *
 * \since v.5.8.0
 */
[[nodiscard]]
inline bool
special_message_sink_ptr_compare(
	const message_sink_t & a,
	const message_sink_t & b ) noexcept
	{
		auto p1 = a.so_message_sink_priority();
		auto p2 = b.so_message_sink_priority();

		// NOTE: there should be inversion -- sink with higher
		// priority must be first.
		return ( p1 > p2 || (p1 == p2 && &a < &b) );
	}

} /* namespace impl */

} /* namespace so_5 */

