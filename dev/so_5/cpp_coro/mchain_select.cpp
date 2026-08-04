/*
 * SObjectizer-5
 */

/// @file
/// @brief Staff related to coroutine-friendly select function.
///
/// @since v.5.8.7

#include <so_5/cpp_coro/mchain_select.hpp>

namespace so_5::cpp_coro
{

namespace async_select_impl
{

//
// abstract_empty_timeout_handler_t
//
abstract_empty_timeout_handler_t::~abstract_empty_timeout_handler_t()
	= default;

void
abstract_empty_timeout_handler_t::noop_deleter(
	abstract_empty_timeout_handler_t * ) noexcept
	{ /* nothing to do */ }

void
abstract_empty_timeout_handler_t::real_deleter(
	abstract_empty_timeout_handler_t * handler ) noexcept
	{
		delete handler;
	}

//
// empty_timeout_handling_tools_t
//
empty_timeout_handling_tools_t::empty_timeout_handling_tools_t( no_timeout_t )
	: m_handler{
			std::addressof(no_empty_timeout_handler),
			&abstract_empty_timeout_handler_t::noop_deleter
		}
	{}

empty_timeout_handling_tools_t::empty_timeout_handling_tools_t(
	empty_timeout_handler_unique_ptr_t handler,
	so_5::mchain_props::select_case_unique_ptr_t select_case )
	: m_handler{ std::move(handler) }
	, m_select_case{ std::move(select_case) }
	{}

//
// no_empty_timeout_handler_t
//
void
no_empty_timeout_handler_t::schedule()
	{
std::cout << "*** no_empty_timeout_handler_t::schedule" << std::endl;
		// Nothing to do.
	}

bool
no_empty_timeout_handler_t::is_elapsed()
	{
std::cout << "*** no_empty_timeout_handler_t::is_elapsed" << std::endl;
		// Nothing to do.
		return false;
	}

//
// actual_no_empty_timeout_handler_t::msg_elapsed
//
actual_no_empty_timeout_handler_t::msg_elapsed::msg_elapsed(
	unsigned long id )
	: m_id{ id }
	{}

//
// actual_no_empty_timeout_handler_t
//
actual_no_empty_timeout_handler_t::actual_no_empty_timeout_handler_t(
	so_5::mchain_t target,
	so_5::mchain_props::duration_t timeout )
	: m_target{ std::move(target) }
	, m_timeout{ timeout }
	, m_actual_id{ 0 }
	, m_is_elapsed{ false }
	{}

actual_no_empty_timeout_handler_t::~actual_no_empty_timeout_handler_t()
	{}

void
actual_no_empty_timeout_handler_t::schedule()
	{
		++m_actual_id;

		m_timer_id = so_5::send_periodic< msg_elapsed >(
				m_target,
				m_timeout,
				so_5::mchain_props::duration_t::zero(),
				m_actual_id );
	}

bool
actual_no_empty_timeout_handler_t::is_elapsed()
	{
		return m_is_elapsed;
	}

void
actual_no_empty_timeout_handler_t::react_to( const msg_elapsed & cmd )
	{
		if( m_actual_id == cmd.m_id )
			{
				m_is_elapsed = true;
			}
		// Otherwise the message will be ignored because it isn't actual
		// any more.
	}

} /* namespace async_select_impl */

resumable_select_t::resumable_select_t( resumable_select_t && other ) noexcept
	: m_coro{ take_out_from( other ) }
	{
	}

resumable_select_t &
resumable_select_t::operator=( resumable_select_t && other ) noexcept
	{
		if( this != std::addressof(other) )
			{
				destroy_if_needed( m_coro );

				m_coro = take_out_from( other );
			}

		return *this;
	}

resumable_select_t::~resumable_select_t()
	{
		destroy_if_needed( m_coro );
	}

resumable_select_t::resumable_select_t(
	std::coroutine_handle< promise_type > coro )
noexcept
	: m_coro{ coro }
	{
	}

std::coroutine_handle< resumable_select_t::promise_type >
resumable_select_t::take_out_from( resumable_select_t & other )
noexcept
	{
		return std::exchange( other.m_coro,
				std::coroutine_handle< promise_type >{} );
	}

void
resumable_select_t::destroy_if_needed(
	std::coroutine_handle< promise_type > coro )
noexcept
	{
		if( coro )
			{
				coro.destroy();
			}
	}

} /* namespace so_5::cpp_coro */

