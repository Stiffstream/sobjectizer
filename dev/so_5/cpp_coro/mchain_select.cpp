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

