/*
 * SObjectizer-5
 */

#include <so_5/cpp_coro/scheduler.hpp>

#include <so_5/exception.hpp>

#include <utility>

namespace so_5::cpp_coro
{

//
// resumable_item_t
//
resumable_item_t::resumable_item_t(
	std::coroutine_handle<> to_be_resumed,
	scheduler_t & scheduler )
	: m_scheduler_data{ to_be_resumed }
	, m_scheduler{ std::addressof(scheduler) }
	{}

resumable_item_t::resumable_item_t( deffered_initialization_t init_marker )
	: m_scheduler_data{ init_marker }
	, m_scheduler{ nullptr }
	{}

resumable_item_t::~resumable_item_t() = default;

void
resumable_item_t::init(
	std::coroutine_handle<> to_be_resumed,
	scheduler_t & scheduler,
	status_t initial_status )
	{
		if( m_scheduler )
			SO_5_THROW_EXCEPTION( rc_resumable_item_already_initialized,
					"deffered_init for already initialized resumable_item" );

		m_scheduler = std::addressof(scheduler);
		m_scheduler_data.m_to_be_resumed = to_be_resumed;
		m_scheduler_data.m_status = initial_status;
	}

void
resumable_item_t::try_schedule()
	{
		if( !m_scheduler )
			SO_5_THROW_EXCEPTION( rc_resumable_item_not_initialized,
					"try_schedule for non-initialized resumable_item" );

		m_scheduler->try_schedule( *this );
	}

try_suspend_result_t
resumable_item_t::try_suspend(
	monotonic_clock_t::duration sleep_time )
	{
		if( !m_scheduler )
			SO_5_THROW_EXCEPTION( rc_resumable_item_not_initialized,
					"try_suspend for non-initialized resumable_item" );

		return m_scheduler->try_suspend( *this, sleep_time );
	}

//
// scheduler_t
//
scheduler_t::scheduler_t() = default;
scheduler_t::~scheduler_t() = default;

} /* namespace so_5::cpp_coro */

