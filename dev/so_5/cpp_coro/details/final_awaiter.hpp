/*
 * SObjectizer-5
 */

/// @file
/// @brief Implementation of final_awaiter_t type.
///
/// @since v.5.8.7

#if !defined( SO_5_CPP_CORO_DETAILS_FINAL_AWAITER_HPP )
#define SO_5_CPP_CORO_DETAILS_FINAL_AWAITER_HPP

#include <coroutine>

namespace so_5::cpp_coro::details
{

/// Concept to check requirements for Promise_Type that can be used
/// with final_awaiter_t.
///
/// @since v.5.8.7
template< typename Promise_Type >
concept promise_type_with_continuation = requires( Promise_Type p )
{
	{ p.query_continuation() } noexcept
		-> std::same_as< std::coroutine_handle<> >;
};

/// Reusable template for final_awaiter that supports symmetric transfer.
///
/// See https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
/// for more details.
///
/// @note
/// The Promise_Type should satisfy promise_type_with_continuation concept.
///
/// @since v.5.8.7
template< typename Promise_Type >
struct final_awaiter_t
{
	bool
	await_ready() noexcept { return false; }

	std::coroutine_handle<>
	await_suspend( std::coroutine_handle< Promise_Type > handle ) noexcept
		{
			static_assert( promise_type_with_continuation<Promise_Type> );

			return handle.promise().query_continuation();
		}

	void
	await_resume() const noexcept {}
};

} /* namespace so_5::cpp_coro::details */

#endif

