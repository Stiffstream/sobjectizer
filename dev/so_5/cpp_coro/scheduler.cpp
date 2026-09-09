/*
 * SObjectizer-5
 */

#include <so_5/cpp_coro/scheduler.hpp>

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

resumable_item_t::~resumable_item_t() = default;

void
resumable_item_t::try_schedule()
	{
		m_scheduler.try_schedule( *this );
	}

//
// scheduler_t
//
scheduler_t::scheduler_t() = default;
scheduler_t::~scheduler_t() = default;

} /* namespace so_5::cpp_coro */

