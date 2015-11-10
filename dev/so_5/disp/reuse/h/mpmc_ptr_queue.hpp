/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Multi-producer/Multi-consumer queue of pointers.
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
		mpmc_ptr_queue_t(
			so_5::disp::mpmc_queue_traits::lock_factory_t lock_factory,
			std::size_t thread_count )
			:	m_lock{ lock_factory() }
			,	m_shutdown( false )
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
								return r;
							}

						m_waiting_customers.push_back( &condition );

						condition.wait();
					}
				while( true );

				return nullptr;
			}

		//! Schedule execution of demands from the queue.
		void
		schedule( T * queue )
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				m_queue.push_back( queue );

				if( !m_waiting_customers.empty() )
					pop_and_notify_one_waiting_customer();
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
		bool	m_shutdown;

		//! Queue object.
		std::deque< T * > m_queue;

		//! Waiting threads.
		std::vector< so_5::disp::mpmc_queue_traits::condition_t * > m_waiting_customers;

		void
		pop_and_notify_one_waiting_customer()
			{
				auto & condition = *m_waiting_customers.back();
				m_waiting_customers.pop_back();

				condition.notify();
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

