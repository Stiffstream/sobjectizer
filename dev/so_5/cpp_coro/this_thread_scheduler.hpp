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
#include <so_5/cpp_coro/task.hpp>

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

/*!
 * @brief Special task template needed for this_thread_scheduler_t.
 *
 * This task type is similar to the standard task_t type, but has
 * important differences:
 *
 * - promise_type::final_suspend returns `std::suspend_always` and
 *   doesn't support symmetric transfer;
 * - top_level_task_t::await_suspend returns `void` and does nothing.
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

} /* namespace this_thread_scheduler_impl */

//
// this_thread_scheduler_t
//
//FIXME: should this class be `final`?
//FIXME: should here be some way to finish the scheduler before the
//completion of the top-level coroutine?
//FIXME: some examples have to be shown in the description of this class.
/*!
 * @brief An implementation of scheduler interface for case when all coroutines
 * have to be scheduler on the context of the current thread.
 *
 * @since v.5.8.7
 */
class SO_5_TYPE this_thread_scheduler_t
	: public scheduler_t
	{
		//! Helper class for managing list of resumable items.
		//!
		//! It implements an double-linked intrusive list of
		//! resumable_item_t instances. Inclusion to the list and
		//! exclusion from the list are noexcept operations.
		class SO_5_TYPE items_list_t
			{
				//! The head of the list.
				resumable_item_t * m_head{ nullptr };

				//! The tail of the list.
				resumable_item_t * m_tail{ nullptr };

			public:
				items_list_t();
				~items_list_t();

				//! Is the list empty?
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

		/*!
		 * @attention
		 * The @a what_to_handle has to be a coroutine that is scheduled
		 * by this scheduler and is running now on the current thread.
		 */
		try_suspend_result_t
		try_suspend(
			resumable_item_t & what_to_handle,
			monotonic_clock_t::duration sleep_time ) override;

		//FIXME: some examples have to be shown in the description.
		/*!
		 * @brief Synchronous execution of the specified task.
		 *
		 * Runs the @a top_level_task (that has to be an awaitable) and returns
		 * only after the completion of the @a top_level_task.
		 *
		 * Rethrows an exception that was thrown during execution of
		 * the @a top_level_task.
		 *
		 * @return the value returned by @a top_level_task.
		 */
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

				//FIXME: should these checks be kept here or is it better to remove
				//them?
				ensure_lists_are_empty();

				return wrapped_top_level.await_resume();
			}

	private:
		/*!
		 * @brief Starts the loop of scheduling ready to resume coroutines.
		 *
		 * It blocks the current thread until @a top_level coroutine reaches
		 * `done` status.
		 */
		void
		wait_and_handle_coroutines(
			//! The top_level coroutine that will be resumed on the first
			//! iteration of the loop. When this coroutine reaches `done`
			//! status the loop will be finished.
			//! It's expected to be an instance of
			//! this_thread_scheduler_impl::top_level_task_t type.
			std::coroutine_handle<> top_level );

		// NOTE: potentially this method shouldn't throw. But it's open
		// question has it be marked as noexcept.
		/*!
		 * @brief Checks waiting coroutines for resumption.
		 *
		 * Checks the m_waiting_items list and extracts those coroutines
		 * for that the sleeping time is elapsed. Such coroutines are moved
		 * into the m_ready_items.
		 *
		 * @note
		 * Has to be called only when the mutex is locked.
		 *
		 * @return the minimal sleeping time for all waiting coroutines or
		 * infinite_speep_time() if the m_waiting_items is empty or if
		 * none of waiting coroutines have actual resume_at values.
		 */
		[[nodiscard]]
		monotonic_clock_t::duration
		check_resumption_time_for_waiting_coroutines();

		/*!
		 * @brief Resumes all coroutines in ready list.
		 *
		 * @note
		 * Has to be called when the mutex is locked. Unlocks the mutex for
		 * the every coroutine resumption and then locks the mutex back.
		 *
		 * @return number of resumed coroutines. It may be 0 if the ready
		 * list is empty.
		 */
		[[nodiscard]]
		std::size_t
		resume_ready_coroutines(
				/// Object for unlocking the mutex temporary.
				std::unique_lock< std::mutex > & locker );

		//FIXME: is this method really needed?
		/*!
		 * @brief Helper method that checks that all lists are empty
		 * at the completion of sync_wait.
		 *
		 * Throws an exception if m_waiting_items or m_ready_items is not empty.
		 *
		 * It's assumed that when top_level task passed to sync_wait method
		 * is completed the all lists should be empty. It this is not the case
		 * then there is an error somewhere and it's better to known about
		 * that error. This method allows to detect such cases.
		 */
		void
		ensure_lists_are_empty() const;
	};

} /* namespace so_5::cpp_coro */

#endif

