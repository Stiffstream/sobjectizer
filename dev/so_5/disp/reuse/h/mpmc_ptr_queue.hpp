/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Multi-producer/Multi-consumer queue of pointers.
 * \since
 * v.5.4.0
 */

#pragma once

#include <so_5/disp/mpmc_queue_traits/h/pub.hpp>

#include <deque>
#include <vector>

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
 * \brief Multi-producer/Multi-consumer queue of pointers.
 *
 * Uses two types of waiting on empty queue:
 * - waiting on spinlock for the limited period of time;
 * - then waiting on heavy synchronization object.
 *
 * \tparam T type of object.
 *
 * \since
 * v.5.4.0
 */
template< class T >
class mpmc_ptr_queue_t
	{
	public :
		mpmc_ptr_queue_t(
			const so_5::disp::mpmc_queue_traits::queue_params_t & queue_params,
			std::size_t thread_count )
			:	m_lock{ queue_params.lock_factory()() }
			,	m_max_thread_count{ thread_count }
			,	m_next_thread_wakeup_threshold{
					queue_params.next_thread_wakeup_threshold() }
			{
				// Reserve some space for storing infos about waiting
				// customer threads.
				m_waiting_customers.reserve( thread_count );
			}

		//! Initiate shutdown for working threads.
		inline void
		shutdown()
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				m_shutdown = true;

				while( !m_waiting_customers.empty() )
					pop_and_notify_one_waiting_customer();
			}

		//! Get next active queue.
		/*!
		 * \retval nullptr is the case of dispatcher shutdown.
		 */
		inline T *
		pop( so_5::disp::mpmc_queue_traits::condition_t & condition )
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				do
					{
						if( m_shutdown )
							break;

						if( !m_queue.empty() )
							{
								auto r = m_queue.front();
								m_queue.pop_front();

								// There could be non-empty queue and sleeping workers...
								try_wakeup_someone_if_possible();

								return r;
							}

						m_waiting_customers.push_back( &condition );

						condition.wait();
						// If we are here then the current wakeup procedure is
						// finished.
						m_wakeup_in_progress = false;
					}
				while( true );

				return nullptr;
			}

		//! Switch the current non-empty queue to another one if it is possible.
		/*!
		 * \since
		 * v.5.5.15.1
		 *
		 * \return nullptr is the case of dispatcher shutdown.
		 */
		inline T *
		try_switch_to_another( T * current ) SO_5_NOEXCEPT
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				if( m_shutdown )
					return nullptr;

				if( !m_queue.empty() )
					{
						auto r = m_queue.front();
						m_queue.pop_front();

						// Old non-empty queue must be stored for further processing.
						// No need to wakup someone because the length of m_queue
						// didn't changed.
						m_queue.push_back( current );

						return r;
					}

				return current;
			}

		//! Schedule execution of demands from the queue.
		void
		schedule( T * queue )
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				m_queue.push_back( queue );

				try_wakeup_someone_if_possible();
			}

		so_5::disp::mpmc_queue_traits::condition_unique_ptr_t
		allocate_condition()
			{
				return m_lock->allocate_condition();
			}

	private :
		//! Object's lock.
		so_5::disp::mpmc_queue_traits::lock_unique_ptr_t m_lock;

		//! Shutdown flag.
		bool	m_shutdown{ false };

		//! Queue object.
		std::deque< T * > m_queue;

		/*!
		 * \since
		 * v.5.5.15.1
		 *
		 * \brief Is some working thread is in wakeup process now.
		 */
		bool	m_wakeup_in_progress{ false };

		/*!
		 * \brief Maximum count of working threads to be used with
		 * that mpmc_queue.
		 *
		 * \since
		 * v.5.5.16
		 */
		const std::size_t m_max_thread_count;

		/*!
		 * \brief Threshold for wake up next working thread if there are
		 * non-empty agent queues.
		 *
		 * \since
		 * v.5.5.16
		 */
		const std::size_t m_next_thread_wakeup_threshold;

		//! Waiting threads.
		std::vector< so_5::disp::mpmc_queue_traits::condition_t * > m_waiting_customers;

		void
		pop_and_notify_one_waiting_customer()
			{
				auto & condition = *m_waiting_customers.back();
				m_waiting_customers.pop_back();

				m_wakeup_in_progress = true;
				condition.notify();
			}

		/*!
		 * \since
		 * v.5.5.15.1
		 *
		 * \brief An attempt to wakeup another sleeping thread is this necessary
		 * and possible.
		 *
		 * \note Since v.5.5.16 there are some changes in wakeup conditions.
		 * A working thread will be woken up if:
		 * - there are something in m_queue;
		 * - there are waiting customers but no one of them is in wakeup now;
		 * - count of items in m_queue is greater than
		 *   m_next_thread_wakeup_threshold or there is no active customers at
		 *   all.
		 */
		void
		try_wakeup_someone_if_possible()
			{
				if( !m_queue.empty() &&
						!m_waiting_customers.empty() &&
						!m_wakeup_in_progress &&
						( m_queue.size() > m_next_thread_wakeup_threshold ||
						m_max_thread_count == m_waiting_customers.size() ) )
					pop_and_notify_one_waiting_customer();
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

