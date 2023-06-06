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

#include <so_5/disp/mpmc_queue_traits/pub.hpp>

#include <deque>
#include <mutex>
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
 * \note
 * Since v.5.8.0 this type implements intrusive queue and requires that
 * type \a T provides the following methods:
 * \code
 * T * intrusive_queue_giveout_next() noexcept;
 * void intrusive_queue_set_next( T * next ) noexcept;
 * \endcode
 *
 * \tparam T type of object.
 *
 * \since v.5.4.0
 */
template< class T >
class mpmc_ptr_queue_t
	{
	public :
		using item_t = T;

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
		shutdown() noexcept
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
		pop( so_5::disp::mpmc_queue_traits::condition_t & condition ) noexcept
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				do
					{
						if( m_shutdown )
							break;

						if( m_head )
							{
								// The queue isn't empty, the head has to be extracted.
								auto r = pop_head();

								// There could be non-empty queue and sleeping workers...
								try_wakeup_someone_if_possible();

								return r;
							}

						// Exception safety note: it seems that there should not be
						// dynamic memory allocation because m_waiting_customers is
						// reserved in the constructor and only push_back and pop_front
						// are used. So if the actual count of worker threads equals
						// to thread_count constructor's parameter, then there is
						// no need to expand m_waiting_customers vector.
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
		 * \return nullptr is the case of dispatcher shutdown.
		 *
		 * \since v.5.5.15.1
		 */
		[[nodiscard]]
		inline T *
		try_switch_to_another( T * current ) noexcept
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				if( m_shutdown )
					return nullptr;

				if( m_head )
					{
						auto r = pop_head();

						// Old non-empty queue must be stored for further processing.
						// No need to wakup someone because the length of the queue
						// didn't changed.
						push_to_queue( current );

						return r;
					}

				return current;
			}

		//! Schedule execution of demands from the queue.
		void
		schedule( T * queue ) noexcept
			{
				std::lock_guard< so_5::disp::mpmc_queue_traits::lock_t > lock{ *m_lock };

				push_to_queue( queue );

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

		/*!
		 * \brief The current head of the intrusive queue.
		 *
		 * Holds nullptr if the queue is empty.
		 *
		 * \since v.5.8.0
		 */
		T * m_head{ nullptr };

		/*!
		 * \brief The current tail of the intrusive queue.
		 *
		 * Holds nullptr if the queue is empty.
		 * It is equal to m_head if the queue contains just one item.
		 *
		 * \since v.5.8.0
		 */
		T * m_tail{ nullptr };

		/*!
		 * \brief The current size of the intrusive queue.
		 *
		 * \since v.5.8.0
		 */
		std::size_t m_queue_size{};

		/*!
		 * \brief Is some working thread is in wakeup process now.
		 *
		 * \since v.5.5.15.1
		 *
		 */
		bool	m_wakeup_in_progress{ false };

		/*!
		 * \brief Maximum count of working threads to be used with
		 * that mpmc_queue.
		 *
		 * \since v.5.5.16
		 */
		const std::size_t m_max_thread_count;

		/*!
		 * \brief Threshold for wake up next working thread if there are
		 * non-empty agent queues.
		 *
		 * \since v.5.5.16
		 */
		const std::size_t m_next_thread_wakeup_threshold;

		//! Waiting threads.
		std::vector< so_5::disp::mpmc_queue_traits::condition_t * > m_waiting_customers;

		void
		pop_and_notify_one_waiting_customer() noexcept
			{
				auto & condition = *m_waiting_customers.back();
				m_waiting_customers.pop_back();

				m_wakeup_in_progress = true;
				condition.notify();
			}

		/*!
		 * \brief An attempt to wakeup another sleeping thread is this necessary
		 * and possible.
		 *
		 * \note Since v.5.5.16 there are some changes in wakeup conditions.
		 * A working thread is awakened when:
		 * - there are something in the queue;
		 * - there are waiting customers but no one of them is in wakeup now;
		 * - count of items in the queue is greater than
		 *   m_next_thread_wakeup_threshold or there is no active customers at
		 *   all.
		 *
		 * \since v.5.5.15.1
		 */
		void
		try_wakeup_someone_if_possible() noexcept
			{
				if( m_head &&
						!m_waiting_customers.empty() &&
						!m_wakeup_in_progress &&
						( m_queue_size > m_next_thread_wakeup_threshold ||
						m_max_thread_count == m_waiting_customers.size() ) )
					{
						pop_and_notify_one_waiting_customer();
					}
			}

		/*!
		 * \brief Helper method that extracts the head item from the queue.
		 *
		 * \attention
		 * This method must only be called if the queue isn't empty.
		 * The method doesn't check this condition by itself.
		 *
		 * \since v.5.8.0
		 */
		[[nodiscard]]
		T *
		pop_head() noexcept
			{
				auto r = m_head;
				m_head = r->intrusive_queue_giveout_next();
				if( !m_head )
					m_tail = nullptr;
				--m_queue_size;

				return r;
			}

		/*!
		 * \brief Helper method that pushes a new item to the end of the queue.
		 *
		 * \since v.5.8.0
		 */
		void
		push_to_queue( T * new_tail ) noexcept
			{
				if( m_tail )
					{
						m_tail->intrusive_queue_set_next( new_tail );
						m_tail = new_tail;
					}
				else
					{
						m_head = m_tail = new_tail;
					}
				++m_queue_size;
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

