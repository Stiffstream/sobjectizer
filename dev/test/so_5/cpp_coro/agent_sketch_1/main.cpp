#include <so_5/all.hpp>

#include <so_5/cpp_coro/scheduler.hpp>
#include <so_5/cpp_coro/mchain_select.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

class this_agent_scheduler_t final
	: so_5::cpp_coro::scheduler_t
{
public:
	struct msg_resume_this final : public so_5::message_t
	{
		so_5::cpp_coro::resumable_item_t * m_what;

		msg_resume_this(
			so_5::cpp_coro::resumable_item_t & what )
			: m_what{ std::addressof(what) }
		{}
	};

private:
	const so_5::mbox_t m_dest;

	std::mutex m_lock;

public:
	this_agent_scheduler_t( so_5::mbox_t dest )
		: m_dest{ std::move(dest) }
	{}

	so_5::environment_t &
	environment() const override
	{
		return m_dest->environment();
	}

	void
	try_schedule(
		so_5::cpp_coro::resumable_item_t & what_to_resume ) override
	{
		std::lock_guard lock{ m_lock };

		switch( what_to_resume.scheduler_data().m_status )
		{
		case so_5::cpp_coro::resumable_item_t::status_t::neutral:
		case so_5::cpp_coro::resumable_item_t::status_t::notified:
			// There is no need to schedule the coroutine because it's
			// running now.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::ready:
			// There is no need to schedule the coroutine because it's
			// already scheduled.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::waiting:
			what_to_resume.scheduler_data().m_status =
					so_5::cpp_coro::resumable_item_t::status_t::ready;

			so_5::send< msg_resume_this >( m_dest, what_to_resume );
		break;
		}
	}

	so_5::cpp_coro::try_suspend_result_t
	try_suspend(
		so_5::cpp_coro::resumable_item_t & what_to_handle,
		so_5::cpp_coro::monotonic_clock_t::duration sleep_time ) override
	{
		std::lock_guard lock{ m_lock };

		switch( what_to_handle.scheduler_data().m_status )
		{
		case so_5::cpp_coro::resumable_item_t::status_t::neutral:
			// This case will be handled outside of the switch.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::notified:
			// Coroutine is running now. The status has to be changed, but
			// coroutine shouldn't be suspended.
			what_to_handle.scheduler_data().m_status =
					so_5::cpp_coro::resumable_item_t::status_t::neutral;

			return so_5::cpp_coro::try_suspend_result_t::should_be_resumed;

		case so_5::cpp_coro::resumable_item_t::status_t::ready:
			// This shouldn't happen.
			throw std::runtime_error{
					"a ready to run coroutine can't be suspended"
				};
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::waiting:
			// This shouldn't happen.
			throw std::runtime_error{
					"try_suspend is called for already suspended coroutine"
				};
		break;
		}

		what_to_handle.scheduler_data().m_status =
				so_5::cpp_coro::resumable_item_t::status_t::waiting;

		if( so_5::cpp_coro::infinite_speep_time() != sleep_time )
			so_5::send_delayed< msg_resume_this >(
					m_dest,
					sleep_time,
					what_to_handle );

		return so_5::cpp_coro::try_suspend_result_t::should_be_suspended;
	}
};

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

