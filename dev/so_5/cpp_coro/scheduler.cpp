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

namespace
{

//! The current scheduler for this thread.
thread_local scheduler_t * g_this_thread_current_scheduler = nullptr;

} /* namespace anonymous */

//
// current_scheduler_setter_t
//
current_scheduler_setter_t::current_scheduler_setter_t(
	scheduler_t & scheduler )
	{
		if( g_this_thread_current_scheduler )
			SO_5_THROW_EXCEPTION( rc_this_thread_scheduler_already_defined,
					"a scheduler for this thread already defined" );

		g_this_thread_current_scheduler = std::addressof(scheduler);
	}

current_scheduler_setter_t::~current_scheduler_setter_t()
	{
		g_this_thread_current_scheduler = nullptr;
	}

scheduler_t &
current_scheduler_setter_t::safe_get()
	{
		if( !g_this_thread_current_scheduler )
			SO_5_THROW_EXCEPTION( rc_this_thread_scheduler_not_defined,
					"there is no scheduler for this thread" );

		return *g_this_thread_current_scheduler;
	}

//
// scoped_scheduler_setter_t
//
scoped_scheduler_setter_t::scoped_scheduler_setter_t(
	scheduler_t & scheduler ) noexcept
		: m_previous_scheduler{ g_this_thread_current_scheduler }
	{
		g_this_thread_current_scheduler = std::addressof(scheduler);
	}

scoped_scheduler_setter_t::~scoped_scheduler_setter_t()
	{
		g_this_thread_current_scheduler = m_previous_scheduler;
	}

} /* namespace so_5::cpp_coro */

