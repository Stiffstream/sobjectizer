/*
 * SObjectizer-5
 */

/// @file
/// @brief Declaration of special "this thread" scheduler for C++20 coroutines.
///
/// @since v.5.8.6

#if !defined( SO_5_CPP_CORO_THIS_THREAD_SCHEDULER_HPP )
#define SO_5_CPP_CORO_THIS_THREAD_SCHEDULER_HPP

#include <so_5/cpp_coro/scheduler.hpp>

#include <so_5/cpp_coro/details/await_result.hpp>

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>

#include <condition_variable>
#include <exception>
#include <mutex>
#include <utility>
#include <variant>

namespace so_5::cpp_coro
{

namespace this_thread_scheduler_impl
{

//FIXME: document this!
struct no_value_t {};

//
// top_level_task_t
//
//FIXME: document this!
template< typename T >
struct top_level_task_t
	{
		struct promise_type;

		std::coroutine_handle< promise_type > m_coro;

		top_level_task_t( std::coroutine_handle< promise_type > coro )
			: m_coro{ coro }
			{
			}

		~top_level_task_t()
			{
				if( m_coro )
				{
					m_coro.destroy();
				}
			}

		top_level_task_t( const top_level_task_t & ) = delete;
		top_level_task_t &
		operator=( const top_level_task_t & ) = delete;

		top_level_task_t( top_level_task_t && ) = delete;
		top_level_task_t &
		operator=( top_level_task_t && ) = delete;

		//FIXME: document this!
		struct promise_type
			{
				std::variant< no_value_t, std::exception_ptr, T > m_result;

				promise_type()
					: m_result{ no_value_t{} }
					{}

				top_level_task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				std::suspend_always
				initial_suspend() noexcept { return {}; }

				std::suspend_always
				final_suspend() noexcept { return {}; }

				template< typename Actual_T >
				void
				return_value( Actual_T && v )
					{
						m_result.template emplace<T>( std::forward<Actual_T>(v) );
					}

				void
				unhandled_exception() noexcept {}
			};

		bool
		await_ready() const noexcept { return false; }

		void
		await_suspend( std::coroutine_handle<> ) noexcept
			{
				m_coro.resume();
			}

		T
		await_resume()
			{
				auto & promise = m_coro.promise();
				switch( promise.m_result.index() )
					{
					case 0:
						SO_5_THROW_EXCEPTION(
								rc_no_value_in_promise_object,
								"no value to return from "
								"top_level_task_t::await_resume" );
					break;

					case 1: std::rethrow_exception(
							std::move(std::get< std::exception_ptr >(
									promise.m_result))
						);
					break;
					}

				return T{ std::move(std::get<T>(promise.m_result)) };
			}
	};

//FIXME: document this!
template<>
struct top_level_task_t< void >
	{
		struct return_void_called_t {};

		struct promise_type;

		std::coroutine_handle< promise_type > m_coro;

		top_level_task_t( std::coroutine_handle< promise_type > coro )
			: m_coro{ coro }
			{
			}

		~top_level_task_t()
			{
				if( m_coro )
				{
					m_coro.destroy();
				}
			}

		top_level_task_t( const top_level_task_t & ) = delete;
		top_level_task_t &
		operator=( const top_level_task_t & ) = delete;

		top_level_task_t( top_level_task_t && ) = delete;
		top_level_task_t &
		operator=( top_level_task_t && ) = delete;

		//FIXME: document this!
		struct promise_type
			{
				std::variant<
						no_value_t,
						std::exception_ptr,
						return_void_called_t >
					m_result;

				promise_type()
					: m_result{ no_value_t{} }
					{}

				top_level_task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				std::suspend_always
				initial_suspend() noexcept { return {}; }

				std::suspend_always
				final_suspend() noexcept { return {}; }

				void
				return_void()
					{
						m_result.template emplace< return_void_called_t >();
					}

				void
				unhandled_exception() noexcept {}
			};

		bool
		await_ready() const noexcept { return false; }

		void
		await_suspend( std::coroutine_handle<> ) noexcept
			{
				m_coro.resume();
			}

		void
		await_resume()
			{
				auto & promise = m_coro.promise();
				switch( promise.m_result.index() )
					{
					case 0:
						SO_5_THROW_EXCEPTION(
								rc_no_value_in_promise_object,
								"no value to return from "
								"top_level_task_t::await_resume" );
					break;

					case 1: std::rethrow_exception(
							std::move(std::get< std::exception_ptr >(
									promise.m_result))
						);
					break;
					}

				// Nothing to return because it's `void`.
			}
	};

} /* namespace this_thread_scheduler_impl */

//
// this_thread_scheduler_t
//
//FIXME: should this class be `final`?
//FIXME: should here be some way to finish the schedule before the
//completion of the top-level coroutine?
//FIXME: document this!
class SO_5_TYPE this_thread_scheduler_t
	: public scheduler_t
	{
		/// Object lock for thread safety.
		std::mutex m_lock;

		/// Condition variable for waiting on empty queue.
		std::condition_variable m_wakeup_cv;

		/// The head of the pending coroutines list.
		///
		/// Value nullptr means that this list is empty.
		resumable_item_t * m_head{ nullptr };

		/// The tail of the pending coroutines list.
		///
		/// Value nullptr means that this list is empty.
		resumable_item_t * m_tail{ nullptr };

	public:
		this_thread_scheduler_t();
		~this_thread_scheduler_t() override;

		void
		schedule( resumable_item_t & what_to_resume ) override;

		//FIXME: document this!
		template< typename Task >
		decltype(auto)
		sync_wait( Task && top_level_task )
			{
				using namespace this_thread_scheduler_impl;

				using T = details::await_result_t<Task>;

				// Wrap the top_level_task into our own coroutine that
				// is in suspended state.
				auto wrapped_top_level =
					[]( Task * t ) -> top_level_task_t<T> {
						co_return co_await *t;
					}( std::addressof(top_level_task) );

				// The dispatcher should have our coroutine in the queue.
				resumable_item_t our_coro{ wrapped_top_level.m_coro };
				this->schedule( our_coro );

				// And now we have to wait for the completion of the
				// top-level task.
				wait_and_handle_coroutines( wrapped_top_level.m_coro );

				return wrapped_top_level.await_resume();
			}

	private:
		//FIXME: document this!
		void
		wait_and_handle_coroutines(
			//FIXME: document this!
			std::coroutine_handle<> top_level );
	};

} /* namespace so_5::cpp_coro */

#endif

