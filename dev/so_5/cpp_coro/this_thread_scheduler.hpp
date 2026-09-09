/*
 * SObjectizer-5
 */

/// @file
/// @brief Declaration of special "this thread" scheduler for C++20 coroutines.
///
/// @since v.5.8.7

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
//FIXME: can so_5::cpp_coro::task_t can be used instead of top_level_task_t
//(or as the base type for top_level_task_t)?
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

				//FIXME: implement this!
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

				//FIXME: implement this!
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
//FIXME: should here be some way to finish the scheduler before the
//completion of the top-level coroutine?
//FIXME: document this!
class SO_5_TYPE this_thread_scheduler_t
	: public scheduler_t
	{
		/// Helper class for managing list of resumable items.
		class items_list_t
			{
				/// The head of the list.
				resumable_item_t * m_head{ nullptr };

				/// The tail of the list.
				resumable_item_t * m_tail{ nullptr };

			public:
				items_list_t();
				~items_list_t();

				/// Is the list empty?
				[[nodiscard]]
				bool
				empty() const noexcept;

				/// Extract the head of the list.
				///
				/// @attention
				/// This method has to be called for non-empty lists only.
				[[nodiscard]]
				resumable_item_t *
				pop_front() noexcept;

				/// Add an item to the end of the list.
				void
				push_back( resumable_item_t & item ) noexcept;

				/// Extract the specified item for the list.
				///
				/// @attention
				/// It's assumed that the @a item is in the list.
				void
				extract( resumable_item_t & item ) noexcept;

				/// Get the head of the list.
				///
				/// @return Pointer the to first item (without extracting it) or
				/// nullptr if the list is empty.
				[[nodiscard]]
				resumable_item_t *
				head() const noexcept;
			};

		/// SObjectizer Environment for that this scheduler is created.
		so_5::environment_t * m_env;

		/// Object lock for thread safety.
		std::mutex m_lock;

		/// Condition variable for waiting on empty queue.
		std::condition_variable m_wakeup_cv;

		/// List of coroutines that wait some external events/notifications.
		items_list_t m_waiting_items;

		/// List of coroutines that are ready to be resumed.
		items_list_t m_ready_items;

	public:
		//FIXME: document this!
		enum class no_environment_case_t { dummy_value };
		//FIXME: document this!
		static constexpr no_environment_case_t no_environment =
				no_environment_case_t::dummy_value;

		//FIXME: document this!
		explicit this_thread_scheduler_t(
			/// SObjectizer Environment for that this scheduler is created.
			so_5::environment_t & env );

		//FIXME: document this!
		explicit this_thread_scheduler_t(
			/// Special marker that tells that scheduler is run without
			/// actual SObjectizer Environment.
			no_environment_case_t );

		~this_thread_scheduler_t() override;

		so_5::environment_t &
		environment() const override;

		void
		try_schedule( resumable_item_t & what_to_resume ) override;

//FIXME: it's assumed that this method will be called only by coroutine
//that is scheduled by this dispatcher. This assumption has to be documented.
		try_suspend_result_t
		try_suspend(
			resumable_item_t & what_to_handle,
			monotonic_clock_t::duration sleep_time ) override;

		//FIXME: document this!
		template< typename Task >
		decltype(auto)
		sync_wait( Task && top_level_task )
			{
				static_assert( so_5::cpp_coro::details::Awaitable< Task > );

				using namespace this_thread_scheduler_impl;

				using T = details::await_result_t<Task>;

				// Wrap the top_level_task into our own coroutine that
				// is in suspended state.
				auto wrapped_top_level =
					[]( Task * t ) -> top_level_task_t<T> {
						co_return co_await *t;
					}( std::addressof(top_level_task) );

				// The dispatcher should have our coroutine in the queue.
				resumable_item_t our_coro{ wrapped_top_level.m_coro, *this };
				our_coro.scheduler_data().m_status =
						resumable_item_t::status_t::ready;
				m_ready_items.push_back( our_coro );

				// And now we have to wait for the completion of the
				// top-level task.
				wait_and_handle_coroutines( wrapped_top_level.m_coro );

			//FIXME: should here be a check that m_waiting_items and
			//m_ready_items are empty?

				return wrapped_top_level.await_resume();
			}

	private:
		//FIXME: document this!
		void
		wait_and_handle_coroutines(
			//FIXME: document this!
			std::coroutine_handle<> top_level );

//FIXME: should this method be noexcept?
		//FIXME: document this!
		/// All coroutines with appropriate resume_at value found will be moved
		/// to ready list.
		///
		/// @note
		/// Has to be called only when the mutex is locked.
		[[nodiscard]]
		monotonic_clock_t::duration
		check_resumption_time_for_waiting_coroutines();

		//FIXME: document this!
		/// @return number of resumed coroutines. It may be 0 if the ready
		/// list is empty.
		[[nodiscard]]
		std::size_t
		resume_ready_coroutines(
				/// Object for unlocking the mutex temporary.
				std::unique_lock< std::mutex > & locker );
	};

} /* namespace so_5::cpp_coro */

#endif

