/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helper functions for working with threads.
 * \since
 * v.5.5.16
 */

#pragma once

#include <array>
#include <thread>

namespace so_5 {

namespace thread_auto_join_details {

/*!
 * \brief Main class for auto join operation.
 *
 * \attention This is Moveable class, not Copyable.
 *
 * \since
 * v.5.5.16
 */
template< std::size_t N >
class auto_joiner_t
	{
		std::array< std::thread *, N > m_threads;

		void
		nullify() { m_threads.fill( nullptr ); }

	public :
		auto_joiner_t( const auto_joiner_t & ) = delete;
		auto_joiner_t & operator=( const auto_joiner_t & ) = delete;

		auto_joiner_t()
			{
				nullify();
			}

		auto_joiner_t( auto_joiner_t && o )
			{
				nullify();
				m_threads.swap( o.m_threads );
			}

		~auto_joiner_t()
			{
				for( auto t : m_threads )
					if( t && t->joinable() )
						t->join();
			}

		friend inline void
		swap( auto_joiner_t & a, auto_joiner_t & b ) SO_5_NOEXCEPT
			{
				a.m_threads.swap( b.m_threads );
			}

		auto_joiner_t &
		operator=( auto_joiner_t && o )
			{
				auto_joiner_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
			}

		void
		set_thread( std::size_t index, std::thread & t )
			{
				m_threads[ index ] = &t;
			}
	};

/*!
 * \brief Helper for filling auto_joiner object.
 *
 * \since
 * v.5.5.16
 */
template< typename Joiner >
void
add_to_joiner( Joiner & to, std::size_t index, std::thread & t )
	{
		to.set_thread( index, t );
	}

/*!
 * \brief Helper for filling auto_joiner object.
 *
 * \since
 * v.5.5.16
 */
template< typename Joiner, typename... Tail >
void
add_to_joiner(
	Joiner & to,
	std::size_t index,
	std::thread & t,
	Tail &&... tail )
	{
		add_to_joiner( to, index, t );
		add_to_joiner( to, index + 1, std::forward< Tail >(tail)... );
	}

} /* namespace thread_auto_join_details */

/*!
 * \brief Helper function for creation of automatic joiner of std::threads.
 *
 * Usage examples:
 * \code
	std::thread first_worker;
	std::thread second_worker;
	auto worker_joiner = so_5::auto_join( first_worker, second_worker );

	first_worker = thread{ ... };
	second_worker = thread{ ... };
	...
 * \endcode
 *
 * \since
 * v.5.5.16
 */
template< typename... Tail >
thread_auto_join_details::auto_joiner_t< 1 + sizeof...(Tail) >
auto_join(
	//! The first thread to be closed.
	std::thread & first_thread,
	//! Other threads to be closed.
	Tail &&... tail )
	{
		using namespace thread_auto_join_details;

		auto_joiner_t< 1 + sizeof...(tail) > joiner;
		add_to_joiner( joiner, 0, first_thread, std::forward< Tail >(tail)... );

		return joiner;
	}

} /* namespace so_5 */


