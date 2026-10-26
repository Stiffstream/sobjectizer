/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff related to special top_level_task
 *
 * \since v.5.8.7
 */

#if !defined( SO_5_CPP_CORO_IMPL_TOP_LEVEL_TASK_HPP )
#define SO_5_CPP_CORO_IMPL_TOP_LEVEL_TASK_HPP

#include <so_5/cpp_coro/task.hpp>

namespace so_5::cpp_coro::details
{

/*!
 * @brief Special task template needed for wrapping a coroutine.
 *
 * This task type is similar to the standard task_t type, but has
 * important differences:
 *
 * - promise_type::final_suspend returns `std::suspend_always` and
 *   doesn't support symmetric transfer;
 * - top_level_task_t::await_suspend returns `void` and does nothing.
 *
 * This type is created to create a coroutine wrapped around an
 * actual user-provided coroutine. This special wrapped coro will be
 * used a special way. For example, it will never be used with
 * co_await statement. Only `resume` and `done` methods will be called
 * for the associated std::coroutine_handle object.
 *
 * This type is used as a thin wrapper around a real awaitable object
 * passed to this_thread_scheduler_t::sync_wait method.
 *
 * @since v.5.8.7
 */
template< typename T >
struct top_level_task_t final : public so_5::cpp_coro::task_t< T >
	{
		using base_type_t = so_5::cpp_coro::task_t< T >;
		using base_type_promise_t = typename base_type_t::promise_type;

		//! Actual promise_type for this task type.
		struct promise_type final : public base_type_promise_t
			{
				top_level_task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				std::suspend_always
				final_suspend() noexcept { return {}; }
			};

		//! Helper method for conversion of coroutine_handle.
		//!
		//! Because top_level_task_t reuses functionality of its base type
		//! it's necessary to pass std::coroutine_handle<base_type_promise_t>
		//! to the base class constructor, but we have
		//! std::coroutine_handle<promise_type>. This method helps convert
		//! between those types.
		[[nodiscard]] static
		std::coroutine_handle< base_type_promise_t >
		to_base_type_coroutine_handle(
				std::coroutine_handle< promise_type > coro ) noexcept
			{
				return std::coroutine_handle< base_type_promise_t >::from_address(
						coro.address() );
			}

		//! Initializing constructor.
		top_level_task_t( std::coroutine_handle< promise_type > coro )
			: base_type_t{ to_base_type_coroutine_handle( coro ) }
			{}

		//! The standard-required method.
		//!
		//! @attention
		//! Does nothing.
		void
		await_suspend( std::coroutine_handle<> ) noexcept
			{
			}
	};

} /* namespace so_5::cpp_coro::details */

#endif

