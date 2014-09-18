/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Multi-producer/Multi-consumer queue of pointers.
 */

#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

#include <so_5/h/spinlocks.hpp>

namespace so_5
{

namespace disp
{

namespace reuse
{

//
// mpmc_ptr_queue_t
//
/*!
 * \since v.5.4.0
 * \brief Multi-producer/Multi-consumer queue of pointers.
 *
 * Uses two types of waiting on empty queue:
 * - waiting on spinlock for the limited period of time;
 * - then waiting on heavy synchronization object.
 *
 * \tparam T type of object.
 */
template< class T >
class mpmc_ptr_queue_t
	{
	public :
		/*!
		 * Type of heavy synchronization object to wait for if
		 * queue is empty.
		 */
		class waiting_object_t
			{
				waiting_object_t( const waiting_object_t & ) = delete;

				waiting_object_t &
				operator=( const waiting_object_t & ) = delete;

				friend class mpmc_ptr_queue_t;

			public :
				inline
				waiting_object_t()
					{}

			private :
				//! Waiting for notification.
				inline void
				wait( std::unique_lock< std::mutex > & l )
					{
						m_condition.wait( l );
					}

				//! Make notification.
				inline void
				lock_and_notify()
					{
						std::lock_guard< std::mutex > l( m_mutex );
						m_condition.notify_one();
					}

			private :
				std::mutex m_mutex;
				std::condition_variable m_condition;
			};

		mpmc_ptr_queue_t()
			:	m_shutdown( false )
			{}

		//! Initiate shutdown for working threads.
		inline void
		shutdown()
			{
				std::lock_guard< default_spinlock_t > lock( m_lock );

				m_shutdown = true;

				while( !m_waiting_threads.empty() )
					pop_and_notify_one_waiting_thread();
			}

		//! Get next active queue.
		/*!
		 * \retval nullptr is the case of dispatcher shutdown.
		 */
		inline T *
		pop( waiting_object_t & wt_alarm )
			{
				using hrc = std::chrono::high_resolution_clock;
				auto spin_stop_point = hrc::now() +
						std::chrono::microseconds( 500 );
				do
					{
						std::unique_lock< default_spinlock_t > lock( m_lock );

						if( m_shutdown )
							break;

						if( !m_queue.empty() )
							{
								auto r = m_queue.front();
								m_queue.pop_front();
								return r;
							}

						if( spin_stop_point <= hrc::now() )
							{
								// Spin loop must be finished.
								// Next waiting must be on heavy waiting object.
								m_waiting_threads.push_back( &wt_alarm );

								std::unique_lock< std::mutex > wt_lock(
										wt_alarm.m_mutex );

								lock.unlock();
								wt_alarm.wait( wt_lock );
							}
						else
							{
								// Spin loop must be continued.
								lock.unlock();
								std::this_thread::yield();
							}
					}
				while( true );

				return nullptr;
			}

		//! Schedule execution of demands from the queue.
		void
		schedule( T * queue )
			{
				std::lock_guard< default_spinlock_t > lock( m_lock );

				m_queue.push_back( queue );

				if( !m_waiting_threads.empty() )
					pop_and_notify_one_waiting_thread();
			}

	private :
		//! Object's lock.
		default_spinlock_t m_lock;

		//! Shutdown flag.
		bool	m_shutdown;

		//! Queue object.
		std::deque< T * > m_queue;

		//! Waiting threads.
		std::deque< waiting_object_t * > m_waiting_threads;

		void
		pop_and_notify_one_waiting_thread()
			{
				waiting_object_t * wt_alarm = m_waiting_threads.front();
				m_waiting_threads.pop_front();

				wt_alarm->lock_and_notify();
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

