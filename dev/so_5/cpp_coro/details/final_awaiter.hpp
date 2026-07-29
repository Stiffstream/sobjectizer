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

//FIXME: a concept that requires presence of query_continuation() method
//in Promise_Type has to be used here!
//
//FIXME: document this!
///
/// See https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
/// for more details.
template< typename Promise_Type >
struct final_awaiter_t
{
	bool
	await_ready() noexcept { return false; }

	std::coroutine_handle<>
	await_suspend( std::coroutine_handle< Promise_Type > handle ) noexcept
		{
			return handle.promise().query_continuation();
		}

	void
	await_resume() const noexcept {}
};

} /* namespace so_5::cpp_coro::details */

#endif

