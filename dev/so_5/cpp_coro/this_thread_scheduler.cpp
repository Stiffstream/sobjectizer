/*
 * SObjectizer-5
 */

/// @file
/// @brief Implementation of special "this thread" scheduler for C++20 coroutines.
///
/// @since v.5.8.7

#include <so_5/cpp_coro/this_thread_scheduler.hpp>

#include <so_5/ret_code.hpp>
#include <so_5/exception.hpp>

#include <utility>

namespace so_5::cpp_coro
{

//
// this_thread_scheduler_t::items_list_t
//
this_thread_scheduler_t::items_list_t::items_list_t() = default;

this_thread_scheduler_t::items_list_t::~items_list_t() = default;

bool
this_thread_scheduler_t::items_list_t::empty() const noexcept
	{
		return nullptr == m_head;
	}

resumable_item_t *
this_thread_scheduler_t::items_list_t::pop_front() noexcept
	{
		resumable_item_t * result = m_head;
		m_head = result->scheduler_data().m_next;
		if( m_head )
			m_head->scheduler_data().m_prev = nullptr;
		else
			{
				m_tail = nullptr;
			}

		return result;
	}

void
this_thread_scheduler_t::items_list_t::push_back(
	resumable_item_t & item ) noexcept
	{
		auto & d = item.scheduler_data();
		d.m_prev = m_tail;
		d.m_next = nullptr;
		m_tail = std::addressof(item);
		if( !m_head )
			m_head = m_tail;
	}

void
this_thread_scheduler_t::items_list_t::extract(
	resumable_item_t & item ) noexcept
	{
		auto & d = item.scheduler_data();
		if( d.m_prev )
			d.m_prev->scheduler_data().m_next = d.m_next;
		else
			m_head = d.m_next;

		if( d.m_next )
			d.m_next->scheduler_data().m_prev = d.m_prev;
		else
			m_tail = d.m_prev;
	}

resumable_item_t *
this_thread_scheduler_t::items_list_t::head() const noexcept
	{
		return m_head;
	}

//
// this_thread_scheduler_t
//
this_thread_scheduler_t::this_thread_scheduler_t(
	so_5::environment_t & env )
	: m_env{ std::addressof(env) }
	{}

this_thread_scheduler_t::this_thread_scheduler_t(
	no_environment_case_t )
	: m_env{ nullptr }
	{}

this_thread_scheduler_t::~this_thread_scheduler_t() = default;

so_5::environment_t &
this_thread_scheduler_t::environment() const
	{
		if( !m_env )
			{
std::cout << "*** no environment_t in this_thread_scheduler_t" << std::endl;
				SO_5_THROW_EXCEPTION( rc_no_soenv_for_cpp_coro_scheduler,
						"so_5::cpp_coro::this_thread_scheduler_t is not "
						"bound to SOEnv instance" );
			}

		return *m_env;
	}

void
this_thread_scheduler_t::try_schedule( resumable_item_t & what_to_resume )
	{
		// All further actions have to be done under locked mutex.
		std::lock_guard< std::mutex > lock{ m_lock };

		bool was_empty = m_ready_items.empty();

		auto & item_data = what_to_resume.scheduler_data();
		switch( item_data.m_status )
			{
			case resumable_item_t::status_t::neutral :
				item_data.m_status = resumable_item_t::status_t::notified;

				// NOTE: we don't push the coroutine to any of lists.
			break;

			case resumable_item_t::status_t::notified :
				// Nothing to be. The item already has the proper status.
			break;

			case resumable_item_t::status_t::waiting :
				item_data.m_status = resumable_item_t::status_t::ready;
				m_waiting_items.extract( what_to_resume );
				m_ready_items.push_back( what_to_resume );
			break;

			case resumable_item_t::status_t::ready:
				// Nothing to do. The item is already in ready list.
			break;
			}

		if( was_empty )
			m_wakeup_cv.notify_one();
	}

try_suspend_result_t
this_thread_scheduler_t::try_suspend(
	resumable_item_t & what_to_handle,
	monotonic_clock_t::duration sleep_time )
	{
		// All further actions have to be done under locked mutex.
		std::lock_guard< std::mutex > lock{ m_lock };

		auto result = try_suspend_result_t::should_be_suspended;

		auto & item_data = what_to_handle.scheduler_data();
		switch( item_data.m_status )
			{
			case resumable_item_t::status_t::neutral :
				item_data.m_status = resumable_item_t::status_t::waiting;

				if( infinite_speep_time() != sleep_time )
					item_data.m_resume_at = monotonic_clock_t::now() + sleep_time;
				else
					item_data.m_resume_at = std::nullopt;

				m_waiting_items.push_back( what_to_handle );
			break;

			case resumable_item_t::status_t::notified :
				// There is no need to suspend the coroutine.
				item_data.m_status = resumable_item_t::status_t::neutral;

				result = try_suspend_result_t::should_be_resumed;
			break;

			case resumable_item_t::status_t::waiting :
				// This should not happen!
				// Something went wrong, it's better to inform about this case.
				SO_5_THROW_EXCEPTION( rc_unexpected_coro_status,
						"coro has 'waiting' status in "
						"this_thread_scheduler_t::try_suspend()" );
			break;

			case resumable_item_t::status_t::ready :
				// This should not happen!
				// Something went wrong, it's better to inform about this case.
				SO_5_THROW_EXCEPTION( rc_unexpected_coro_status,
						"coro has 'ready' status in "
						"this_thread_scheduler_t::try_suspend()" );
			break;
			}

		return result;
	}

void
this_thread_scheduler_t::wait_and_handle_coroutines(
	std::coroutine_handle<> top_level )
	{
		// Acquire the mutex now and unlock it temporary when it's needed.
		std::unique_lock lock{ m_lock };

		// Work has to be continued while the top_level coroutine is not
		// finished yet.
		while( !top_level.done() )
			{
				// Check waiting coroutines for resumption time.
				monotonic_clock_t::duration sleep_time =
						check_resumption_time_for_waiting_coroutines();

				std::size_t coroutines_resumed = 0;
				if( !m_ready_items.empty() )
					coroutines_resumed = resume_ready_coroutines( lock );

				// NOTE: if a coroutine was resumed then we don't sleep and
				// have to check the waiting list again (and recalculate
				// sleep_time appropriately).
				if( !coroutines_resumed )
					// Have to wait while one of coroutines become ready.
					m_wakeup_cv.wait_for( lock, sleep_time );
			}
	}

monotonic_clock_t::duration
this_thread_scheduler_t::check_resumption_time_for_waiting_coroutines()
	{
		monotonic_clock_t::duration sleep_time = infinite_speep_time();

		// NOTE: will be set for the first coroutine with non-empty
		// m_resume_at.
		std::optional< monotonic_clock_t::time_point > current_time;

		resumable_item_t * item = m_waiting_items.head();
		while( item )
			{
				auto & item_data = item->scheduler_data();

				// Store the next pointer now because m_next may be changed
				// if the item is moved into ready list.
				resumable_item_t * next = item_data.m_next;

				if( item_data.m_resume_at.has_value() )
					{
						if( !current_time.has_value() )
							current_time = monotonic_clock_t::now();

						if( *item_data.m_resume_at <= *current_time )
							{
								// This coroutine has to be moved to the ready list.
								m_waiting_items.extract( *item );

								// The coroutine to be resumed must have a special status.
								item_data.m_status = resumable_item_t::status_t::ready;
								m_ready_items.push_back( *item );
							}
						else
							{
								// Sleep time has to be updated.
								const auto d = *item_data.m_resume_at - *current_time;
								if( sleep_time > d )
									sleep_time = d;
							}
					}

				item = next;
			}

		return sleep_time;
	}

std::size_t
this_thread_scheduler_t::resume_ready_coroutines(
		std::unique_lock< std::mutex > & locker )
	{
		std::size_t count = 0;

		resumable_item_t * item = m_ready_items.head();

		while( item )
			{
				++count;

				m_ready_items.extract( *item );
				auto & item_data = item->scheduler_data();
				// The status has to be changed while mutex is locked.
				item_data.m_status = resumable_item_t::status_t::neutral;

				// NOTE: if resume() throws then locker won't be locked back.
				locker.unlock();

				// NOTE: the status can be changes to status_t::notified by a
				// parallel thread.
				item_data.m_to_be_resumed.resume();

				locker.lock();

				item = m_ready_items.head();
			}

		return count;
	}

} /* namespace so_5::cpp_coro */

