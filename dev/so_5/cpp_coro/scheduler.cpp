/*
 * SObjectizer-5
 */

#include <so_5/cpp_coro/scheduler.hpp>

//FIXME: remove after debugging!
#include <iostream>

namespace so_5::cpp_coro
{

//
// resumable_item_t
//
resumable_item_t::resumable_item_t(
	std::coroutine_handle<> to_be_resumed,
	scheduler_t & scheduler )
	: m_scheduler_data{ to_be_resumed }
	, m_scheduler{ scheduler }
	{}

#if 0
resumable_item_t::~resumable_item_t() = default;
#else
resumable_item_t::~resumable_item_t()
{
	std::cout << "~resumable_item_t " << this << std::endl;
}
#endif

void
resumable_item_t::try_schedule()
	{
		m_scheduler.try_schedule( *this );
	}

try_suspend_result_t
resumable_item_t::try_suspend(
	monotonic_clock_t::duration sleep_time )
	{
		return m_scheduler.try_suspend( *this, sleep_time );
	}

//
// scheduler_t
//
scheduler_t::scheduler_t() = default;
scheduler_t::~scheduler_t() = default;

} /* namespace so_5::cpp_coro */

