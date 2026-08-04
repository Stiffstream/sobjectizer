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
this_thread_scheduler_t::schedule( resumable_item_t & what_to_resume )
	{
		bool was_empty = false;

		std::lock_guard< std::mutex > lock{ m_lock };
		if( !m_tail )
			{
				m_head = m_tail = std::addressof(what_to_resume);
				was_empty = true;
			}
		else
			{
				m_tail->m_next = std::addressof(what_to_resume);
				m_tail = m_tail->m_next;
			}

		if( was_empty )
			m_wakeup_cv.notify_one();
	}

void
this_thread_scheduler_t::wait_and_handle_coroutines(
	std::coroutine_handle<> top_level )
	{
		// Work has to be continued while the top_level coroutine is not
		// finished yet.
		while( !top_level.done() )
			{
				auto * to_resume =
					[this]() -> resumable_item_t *
					{
						std::unique_lock< std::mutex > lock{ m_lock };

						if( !m_head )
							m_wakeup_cv.wait( lock,
									[this]() {
										return nullptr != m_head;
									} );

						resumable_item_t * result = m_head;
						m_head = m_head->m_next;
						if( !m_head )
							m_tail = nullptr;

						return result;
					}();

				to_resume->m_to_be_resumed.resume();
			}
	}

} /* namespace so_5::cpp_coro */

