/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief An implementation of thread pool dispatcher.
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <memory>
#include <map>
#include <iostream>
#include <atomic>

#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/disp.hpp>

#include <so_5/disp/reuse/h/mpmc_ptr_queue.hpp>

#include <so_5/disp/thread_pool/impl/h/common_implementation.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace impl
{

using spinlock_t = so_5::default_spinlock_t;

class agent_queue_t;

//
// dispatcher_queue_t
//
using dispatcher_queue_t = so_5::disp::reuse::mpmc_ptr_queue_t< agent_queue_t >;

//
// agent_queue_t
//
/*!
 * \since v.5.4.0
 * \brief Event queue for the agent (or cooperation).
 */
class agent_queue_t
	:	public so_5::rt::event_queue_t
	,	private so_5::atomic_refcounted_t
	{
		friend class so_5::intrusive_ptr_t< agent_queue_t >;

	private :
		//! Actual demand in event queue.
		struct demand_t : public so_5::rt::execution_demand_t
			{
				//! Next item in queue.
				demand_t * m_next;

				demand_t()
					:	m_next( nullptr )
					{}
				demand_t( so_5::rt::execution_demand_t && original )
					:	so_5::rt::execution_demand_t( std::move( original ) )
					,	m_next( nullptr )
					{}
			};

	public :
		//! Constructor.
		agent_queue_t(
			//! Dispatcher queue to work with.
			dispatcher_queue_t & disp_queue,
			//! Parameters for the queue.
			const params_t & params )
			:	m_disp_queue( disp_queue )
			,	m_max_demands_at_once( params.query_max_demands_at_once() )
			,	m_tail( &m_head )
			{}

		~agent_queue_t()
			{
				while( m_head.m_next )
					delete_head();
			}

		//! Push next demand to queue.
		virtual void
		push( so_5::rt::execution_demand_t demand )
			{
				std::unique_ptr< demand_t > tail_demand{
						new demand_t( std::move( demand ) ) };

				std::lock_guard< spinlock_t > lock( m_lock );

				bool was_empty = (nullptr == m_head.m_next);

				m_tail->m_next = tail_demand.release();
				m_tail = m_tail->m_next;

				++m_size;

				if( was_empty )
					m_disp_queue.schedule( this );
			}

		//! Get the front demand from queue.
		/*!
		 * \attention This method must be called only on non-empty queue.
		 */
		so_5::rt::execution_demand_t &
		front()
			{
				return *(m_head.m_next);
			}

		//! Remove the front demand.
		/*!
		 * \retval true queue not empty and next demand can be processed.
		 * \retval false queue empty or
		 * demands_processed >= m_max_demands_at_once
		 */
		bool
		pop(
			//! Count of consequently processed demands from that queue.
			std::size_t demands_processed )
			{
				std::lock_guard< spinlock_t > lock( m_lock );

				delete_head();

				if( m_head.m_next )
				{
					if( demands_processed < m_max_demands_at_once )
						return true;
					else
						m_disp_queue.schedule( this );
				}
				else
					m_tail = &m_head;

				return false;
			}

		/*!
		 * \brief Wait while queue becomes empty.
		 *
		 * It is necessary because there is a possibility that
		 * after processing of demand_handler_on_finish cooperation
		 * will be destroyed and agents will be unbound from dispatcher
		 * before the return from demand_handler_on_finish.
		 *
		 * Without waiting for queue emptyness it could lead to
		 * dangling pointer to agent_queue in woring thread.
		 */
		void
		wait_for_emptyness()
			{
				bool empty = false;
				while( !empty )
					{
						{
							std::lock_guard< spinlock_t > lock( m_lock );
							empty = (nullptr == m_head.m_next);
						}

						if( !empty )
							std::this_thread::yield();
					}
			}

		/*!
		 * \since v.5.5.4
		 * \brief Get the current size of the queue.
		 */
		std::size_t
		size() const
			{
				return m_size.load( std::memory_order_acquire );
			}

	private :
		//! Dispatcher queue for scheduling processing of events from
		//! this queue.
		dispatcher_queue_t & m_disp_queue;

		//! Maximum count of demands to be processed consequently.
		const std::size_t m_max_demands_at_once;

		//! Object's lock.
		spinlock_t m_lock;

		//! Head of the demand's queue.
		/*!
		 * Never contains actual demand. Only m_next field is used.
		 */
		demand_t m_head;
		//! Tail of the demand's queue.
		/*!
		 * Must point to m_head if queue is empty or to the very
		 * last queue item otherwise.
		 */
		demand_t * m_tail;

		/*!
		 * \since v.5.5.4
		 * \brief Current size of the queue.
		 */
		std::atomic< std::size_t > m_size = { 0 };

		//! Helper method for deleting queue's head object.
		inline void
		delete_head()
			{
				auto to_be_deleted = m_head.m_next;
				m_head.m_next = m_head.m_next->m_next;

				--m_size;

				delete to_be_deleted;
			}
	};

//
// work_thread_t
//
/*!
 * \since v.5.4.0
 * \brief Class of work thread for thread pool dispatcher.
 */
class work_thread_t
	{
	public :
		//! Initializing constructor.
		work_thread_t( dispatcher_queue_t & queue )
			:	m_disp_queue( &queue )
			{
			}

		void
		join()
			{
				m_thread.join();
			}

		//! Launch work thread.
		void
		start()
			{
				m_thread = std::thread( [this]() { body(); } );
			}

	private :
		//! Dispatcher's queue.
		dispatcher_queue_t * m_disp_queue;

		//! ID of thread.
		/*!
		 * Receives actual value inside body().
		 */
		so_5::current_thread_id_t m_thread_id;

		//! Actual thread.
		std::thread m_thread;

		//! Waiting object for long wait.
		dispatcher_queue_t::waiting_object_t m_waiting_object;

		//! Thread body method.
		void
		body()
			{
				m_thread_id = so_5::query_current_thread_id();

				agent_queue_t * agent_queue;
				while( nullptr !=
						(agent_queue = m_disp_queue->pop( m_waiting_object )) )
					{
						process_queue( *agent_queue );
					}
			}

		//! Processing of demands from agent queue.
		void
		process_queue( agent_queue_t & queue )
			{
				std::size_t demands_processed = 0;
				bool need_continue = true;

				while( need_continue )
					{
						auto & d = queue.front();

						d.call_handler( m_thread_id );

						++demands_processed;
						need_continue = queue.pop( demands_processed );
					}
			}
	};

//
// adaptation_t
//
/*!
 * \since v.5.5.4
 * \brief Adaptation of common implementation of thread-pool-like dispatcher
 * to the specific of this thread-pool dispatcher.
 */
struct adaptation_t
	{
		static const char *
		dispatcher_type_name()
			{
				return "tp"; // thread_pool.
			}

		static bool
		is_individual_fifo( const params_t & params )
			{
				return fifo_t::individual == params.query_fifo();
			}

		static void
		wait_for_queue_emptyness( agent_queue_t & queue )
			{
				queue.wait_for_emptyness();
			}
	};

//
// dispatcher_t
//
/*!
 * \since v.5.4.0
 * \brief Actual type of this thread-pool dispatcher.
 */
using dispatcher_t =
		common_implementation::dispatcher_t<
				work_thread_t,
				dispatcher_queue_t,
				agent_queue_t,
				params_t,
				adaptation_t >;

} /* namespace impl */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */

