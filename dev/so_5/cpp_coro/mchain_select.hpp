/*
 * SObjectizer-5
 */

/// @file
/// @brief Staff related to coroutine-friendly select function.
///
/// @since v.5.8.7

#if !defined( SO_5_CPP_CORO_MCHAIN_SELECT_HPP )
#define SO_5_CPP_CORO_MCHAIN_SELECT_HPP

#include <so_5/declspec.hpp>

#include <so_5/cpp_coro/details/final_awaiter.hpp>
#include <so_5/cpp_coro/details/current_coro_handle.hpp>

#include <so_5/cpp_coro/scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <so_5/details/remaining_time_counter.hpp>

#include <so_5/message.hpp>
#include <so_5/mchain_select.hpp>
#include <so_5/timers.hpp>
#include <so_5/environment.hpp>
#include <so_5/send_functions.hpp>

#include <exception>

namespace so_5::cpp_coro
{

//
// resumable_select_t
//
/// @brief An alias for the result of asynchronous select.
///
/// @since v.5.8.7
using resumable_select_t = task_t< mchain_select_result_t >;

namespace async_select_impl
{

/// @brief Implementation of select_notificator for asynchronous select.
///
/// The notificator also holds an instance of resumable_item_t to be
/// use for coroutine resumption by the scheduler.
///
/// @attention
/// It's expected that the reference to the scheduler remains valid for
/// the whole lifetime of the notificator.
///
/// @since v.5.8.7
class async_select_notificator_t final
	: public so_5::mchain_props::select_notificator_t
	{
	private :
		//! Object's lock for thread-safety.
		std::mutex m_lock;

		//! Information for resumption.
		resumable_item_t m_to_be_resumed;

		//! Queue of already notified select_cases.
		so_5::mchain_props::select_case_t * m_tail = nullptr;

		/*!
		 * \attention This method must be called only on locked object.
		 */
		void
		push_to_notified_chain(
			so_5::mchain_props::select_case_t & what ) noexcept
			{
				what.set_next( m_tail );
				m_tail = &what;
			}

	public :
		/*!
		 * \brief Initializing constructor.
		 *
		 * Intended to be used with select_cases_holder and its iterators.
		 *
		 * Every select_case is automatically added to the list of notified
		 * select_cases.
		 */
		template< typename Fwd_it >
		async_select_notificator_t(
			//! The scheduler to be used for the coroutine resumption.
			scheduler_t & scheduler,
			//! The coroutine to be resumed when notificator will be activated.
			std::coroutine_handle<> coro_to_resume,
			//! The begin interator of the list of select_cases.
			Fwd_it b,
			//! The end (past-the-end) iterator of the list of select_cases.
			Fwd_it e )
			: m_to_be_resumed{ coro_to_resume, scheduler }
			{
				// All select_cases from range [b,e) must be included in
				// ready_cases list.
				while( b != e )
					{
						b->set_next( m_tail );
						m_tail = &(*b);
						++b;
					}
			}

		void
		notify(
			so_5::mchain_props::select_case_t & what ) noexcept override
			{
				so_5::mchain_props::select_case_t * old_tail = nullptr;
				{
					std::lock_guard< std::mutex > lock{ m_lock };

					old_tail = m_tail;
					push_to_notified_chain( what );
				}

				if( !old_tail )
					m_to_be_resumed.try_schedule();
			}

		/*!
		 * \brief Return specifed select_case object to the chain of
		 * 'notified select_cases'.
		 *
		 * If a message has been read from a mchain then there could be
		 * other messages in that mchain. Because of that the select_case
		 * for that mchain must be seen as 'notified' -- it should be
		 * processed on next call to wait() method. This method must be
		 * used for immediately return of select_case to the chain of
		 * 'notified select_cases'.
		 */
		void
		return_to_ready_chain(
			so_5::mchain_props::select_case_t & what ) noexcept
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				push_to_notified_chain( what );
			}

		//! Initiate waiting for events.
		/*!
		 * This is analogue of actual_select_notificator_t for synchronous
		 * select, but doesn't receive wait time parameter, because this method
		 * doesn't wait anything. It just got the list of ready select_cases and
		 * returns this list.
		 *
		 * @retval nullptr if there is no ready to resume select_cases.
		 */
		[[nodiscard]]
		so_5::mchain_props::select_case_t *
		wait()
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				auto * result = m_tail;
				m_tail = nullptr;

				return result;
			}

		//! Get access to the resumable_item_t object.
		[[nodiscard]]
		resumable_item_t &
		resumable_item() noexcept
			{
				return m_to_be_resumed;
			}
	};

/*!
 * An alias for mchain_select_params_t with defined number of expected
 * messages.
 *
 * @since v.5.8.7
 */
using defined_select_params_t =
		mchain_select_params_t< so_5::mchain_props::msg_count_status_t::defined >;

/*!
 * @brief The actual implementation of asynchronous select.
 *
 * @tparam Total_Time_Counter type to be used for counter of total execution
 * time.
 *
 * @tparam Cases_Count how many select_cases are used in the select call.
 *
 * @since v.5.8.7
 */
template<
	class Total_Time_Counter,
	std::size_t Cases_Count >
resumable_select_t
do_select(
	/// Scheduler to be used for coroutine resumption.
	scheduler_t & coro_scheduler,
	/// Parameters for the select call.
	defined_select_params_t params,
	/// Container with select_cases to be used.
	so_5::mchain_props::details::select_cases_holder_t< Cases_Count > cases_holder )
	{
		using namespace so_5::details;
		using namespace so_5::mchain_props::details;

		using holder_t = select_cases_holder_t< Cases_Count >;

		using performer_t = select_actions_performer_t<
				holder_t,
				async_select_notificator_t >;

		async_select_notificator_t notificator{
				coro_scheduler,
				co_await so_5::cpp_coro::details::current_coro_handle(),
				cases_holder.begin(),
				cases_holder.end()
			};

		performer_t performer{
				params,
				cases_holder,
				notificator
			};

		Total_Time_Counter total_time_counter{ params.total_time() };
		remaining_time_counter_t wait_incoming_time{ params.empty_timeout() };
		bool wait_incoming_time_not_elapsed_yet = true;

		do
			{
				const auto handle_result = co_await performer.handle_next_async();

				if( so_5::mchain_props::extraction_status_t::msg_extracted ==
						performer.last_extraction_status() )
					{
						// Becase some message extracted we must restart
						// wait_incoming_time counting.
						wait_incoming_time =
								remaining_time_counter_t{ params.empty_timeout() };

						wait_incoming_time_not_elapsed_yet = true;
					}
				else
					{
						// Otherwise wait_incoming_time should be updated to
						// reduce it value.
						wait_incoming_time.update();
						wait_incoming_time_not_elapsed_yet =
								static_cast<bool>(wait_incoming_time);

						// total_time_counter has to be updated to because
						// handle_next_async may take some time.
						total_time_counter.update();

						// If there weren't ready cases then we have to wait.
						// But because it's an asynchronous select then we have
						// to suspend the coroutine for some time.
						if( handle_next_result_t::no_ready_cases == handle_result )
							{
								// Ask for awaitable object that is bound to
								// appropriate wait time and wait until this
								// awaitable resumes.
								co_await try_suspend_for(
										notificator.resumable_item(),
										// Sleeping time has to be limited with respect
										// to empty_timeout and total_time.
										std::min(
												total_time_counter.remaining(),
												wait_incoming_time.remaining() ) );
							}
					}

				// total_time_counter has to be updated again because of potential
				// sleeping in else-branch.
				total_time_counter.update();
			}
		while( total_time_counter
				&& wait_incoming_time_not_elapsed_yet
				&& performer.can_continue() );

		co_return performer.make_result();
	}

/// @brief Helper function to initiate the call of do_select.
///
/// Detects the presence of total_time in @a params and calls do_select
/// with fake_time_counter_t or normal_time_counter_t.
///
/// @since v.5.8.7
template<
	std::size_t Cases_Count >
resumable_select_t
perform_select(
	/// Scheduler to be used.
	scheduler_t & coro_scheduler,
	/// Parameters for the select call.
	defined_select_params_t params,
	/// Container with select_cases to be used.
	so_5::mchain_props::details::select_cases_holder_t< Cases_Count > cases_holder )
	{
		using normal_time_counter_t = so_5::details::remaining_time_counter_t;
		using fake_time_counter_t = so_5::details::fake_remaining_time_counter_t;

		if( so_5::mchain_props::details::is_infinite_wait_timevalue(
				params.total_time() ) )
			return do_select< fake_time_counter_t >(
					coro_scheduler,
					std::move(params),
					std::move(cases_holder) );
		else
			return do_select< normal_time_counter_t >(
					coro_scheduler,
					std::move(params),
					std::move(cases_holder) );
	}

} /* namespace async_select_impl */

//FIXME: document this!
/*!
 * @since v.5.8.7
 */
template<
	so_5::mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Cases >
resumable_select_t
select(
	//! The scheduler to be used for coroutine resumption.
	scheduler_t & coro_scheduler,
	//! Parameters for advanced select.
	so_5::mchain_select_params_t< Msg_Count_Status > params,
	//! Select cases.
	Cases &&... cases )
	{
		static_assert(
				Msg_Count_Status == so_5::mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		using namespace so_5::mchain_props;
		using namespace so_5::mchain_props::details;

		select_cases_holder_t< sizeof...(cases) > cases_holder;
		fill_select_cases_holder(
				cases_holder, 0, std::forward< Cases >(cases)... );

		return async_select_impl::perform_select(
				coro_scheduler,
				std::move(params),
				std::move(cases_holder) );
	}

} /* namespace so_5::cpp_coro */

#endif

