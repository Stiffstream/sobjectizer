/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief Definition of various types of spinlocks.
 */

#pragma once

#include <atomic>
#include <thread>
#include <cstdint>

namespace so_5
{

//
// yield_backoff_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief A implementation of backoff object with usage of std::yield.
 */
class yield_backoff_t
	{
	public :
		inline void
		operator()()
			{
				std::this_thread::yield();
			}
	};

//
// spinlock_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief A simple spinlock (analog of std::mutex).
 *
 * This implemetation in based on description of std::atomic_flag:
 * \see http://en.cppreference.com/w/cpp/atomic/atomic_flag
 */
template< class Backoff >
class spinlock_t
	{
	public :
		spinlock_t()
			{
				m_flag.clear( std::memory_order_release );
			}
		spinlock_t( const spinlock_t & ) = delete;
		spinlock_t( spinlock_t && ) = delete;

		spinlock_t & operator=( const spinlock_t & ) = delete;
		spinlock_t & operator=( spinlock_t && ) = delete;

		//! Lock object.
		void
		lock()
			{
				Backoff backoff;
				while( m_flag.test_and_set( std::memory_order_acquire ) )
					backoff();
			}

		//! Unlock object.
		void
		unlock()
			{
				m_flag.clear( std::memory_order_release );
			}

	private :
		//! Atomic flag which is used as actual lock.
		std::atomic_flag m_flag;
	};

//
// default_spinlock_t
//
typedef spinlock_t< yield_backoff_t > default_spinlock_t;

//
// rw_spinlock_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief A simple multi-readers/single-writer spinlock
 * (analog of std::shared_mutex).
 *
 * This implementation is based on Dmitry Vyukov implementation
 * from LLVM code base:
 * http://llvm.org/viewvc/llvm-project/compiler-rt/trunk/lib/tsan/rtl/tsan_mutex.cc?revision=210345&view=markup
 */
template< class Backoff >
class rw_spinlock_t
	{
	private :
		std::atomic_uint_fast32_t m_counters;

		static const std::uint_fast32_t unlocked = 0;
		static const std::uint_fast32_t write_lock = 1;
		static const std::uint_fast32_t read_lock = 2;

	public :
		rw_spinlock_t()
			{
				m_counters.store( unlocked, std::memory_order_release );
			}
		rw_spinlock_t( const rw_spinlock_t & ) = delete;

		rw_spinlock_t & operator=( const rw_spinlock_t & ) = delete;

		//! Lock object in shared mode.
		inline void
		lock_shared()
			{
				Backoff backoff;

				std::uint_fast32_t previous = m_counters.fetch_add(
						read_lock,
						std::memory_order_acquire );
				while( previous & write_lock )
					{
						backoff();

						previous = m_counters.load( std::memory_order_acquire );
					}
			}

		//! Unlock object locked in shared mode.
		inline void
		unlock_shared()
			{
				m_counters.fetch_sub( read_lock, std::memory_order_release );
			}

		//! Lock object in exclusive mode.
		inline void
		lock()
			{
				std::uint_fast32_t expected = unlocked;
				const std::uint_fast32_t desired = write_lock;

				if( m_counters.compare_exchange_strong(
						expected, desired,
						std::memory_order_acquire,
						std::memory_order_relaxed ) )
					return;

				Backoff backoff;

				while( true )
					{
						if( unlocked == m_counters.load( std::memory_order_relaxed ) )
						{
							expected = unlocked;

							if( m_counters.compare_exchange_weak(
									expected, desired,
									std::memory_order_acquire,
									std::memory_order_relaxed ) )
								break;
						}

						backoff();
					}
			}

		//! Unlock object locked in exclusive mode.
		inline void
		unlock()
			{
				m_counters.fetch_sub( write_lock, std::memory_order_release );
			}
	};

typedef rw_spinlock_t< yield_backoff_t > default_rw_spinlock_t;

//
// read_lock_guard_t
//
/*!
 * \since
 * v.5.4.0
 *
 * \brief Scoped guard for shared locks.
 */
template< class Lock >
class read_lock_guard_t
	{
	private :
		Lock & m_lock;

	public :
		read_lock_guard_t( Lock & l ) : m_lock( l )
			{
				m_lock.lock_shared();
			}
		~read_lock_guard_t()
			{
				m_lock.unlock_shared();
			}

		read_lock_guard_t( const read_lock_guard_t & ) = delete;
		read_lock_guard_t &
		operator=( const read_lock_guard_t & ) = delete;
	};

} /* namespace so_5 */

