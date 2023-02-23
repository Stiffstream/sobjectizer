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

#include <functional>

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
	const message_sink_t * a,
	const message_sink_t * b ) noexcept
	{
		// Use std::less for compare pointers.
		// NOTE: it's UB to compare pointers that do not belong to the same array.
		using ptr_comparator_t = std::less< const message_sink_t * >;

		const auto priority_getter = [](auto * sink) noexcept {
				return sink == nullptr ? so_5::priority_t::p_min : sink->sink_priority();
			};
		const auto p1 = priority_getter( a );
		const auto p2 = priority_getter( b );

		// NOTE: there should be inversion -- sink with higher
		// priority must be first.
		return ( p1 > p2 || (p1 == p2 && ptr_comparator_t{}( a, b ) ) );
	}

} /* namespace impl */

} /* namespace so_5 */

