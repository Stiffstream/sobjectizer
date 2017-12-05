/*
	SObjectizer 5.
*/

/*!
 * \since
 * v.5.5.8
 *
 * \file
 * \brief A demand queue for dispatcher with one common working
 * thread and round-robin processing of prioritised demand on
 * quoted basic.
 */

#pragma once

#include <memory>
#include <atomic>

#include <so_5/rt/h/execution_demand.hpp>
#include <so_5/rt/h/event_queue.hpp>

#include <so_5/h/priority.hpp>

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>

#include <so_5/disp/prio_one_thread/quoted_round_robin/h/quotes.hpp>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace quoted_round_robin {

namespace impl {

namespace queue_traits = so_5::disp::mpsc_queue_traits;

//
// demand_t
//
/*!
 * \since
 * v.5.5.8
 *
 * \brief A single execution demand.
 */
struct demand_t : public execution_demand_t
	{
		//! Next demand in the queue.
		demand_t * m_next = nullptr;

		//! Initializing constructor.
		demand_t( execution_demand_t && source )
			:	execution_demand_t( std::move( source ) )
			{}
	};

//
// demand_unique_ptr_t
//
/*!
 * \since
 * v.5.5.8
 *
 * \brief An alias for unique_ptr to demand.
 */
using demand_unique_ptr_t = std::unique_ptr< demand_t >;

//
// demand_queue_t
//
/*!
 * \since
 * v.5.5.8
 * 
 * \brief A demand queue for dispatcher with one common working
 * thread and round-robin processing of prioritised demand on
 * quoted basic.
 */
class demand_queue_t
	{
		friend struct queue_for_one_priority_t;

		//! Description of queue for one priority.
		struct queue_for_one_priority_t
			:	public event_queue_t
			{
				//! Pointer to main demand queue.
				demand_queue_t * m_demand_queue = nullptr;

				//! Head of the queue.
				/*! Null if queue is empty. */
				demand_t * m_head = nullptr;
				//! Tail of the queue.
				/*! Null if queue is empty. */
				demand_t * m_tail = nullptr;

				//! A quote for this subqueue.
				/*!
				 * \note Actual value will be set later in the constructor
				 * of demand_queue_t.
				 */
				std::size_t m_quote = 0;
				//! Count of processed demands on the current iterations.
				std::size_t m_demands_processed = 0;

				/*!
				 * \name Information for run-time monitoring.
				 * \{
				 */
				//! Count of agents attached to that queue.
				std::atomic< std::size_t > m_agents_count = { 0 };
				//! Count of demands in the queue.
				std::atomic< std::size_t > m_demands_count = { 0 };
				/*!
				 * \}
				 */

				virtual void
				push( execution_demand_t exec_demand ) override
					{
						demand_unique_ptr_t what{ new demand_t{
								std::move( exec_demand ) } };

						m_demand_queue->push( this, std::move( what ) );
					}
			};

	public :
		//! This exception is thrown when pop is called after stop.
		class shutdown_ex_t : public std::exception
			{};

		//! Statistic about one subqueue.
		struct queue_stats_t
			{
				priority_t m_priority;
				std::size_t m_quote;
				std::size_t m_agents_count;
				std::size_t m_demands_count;
			};

		demand_queue_t(
			queue_traits::lock_unique_ptr_t lock,
			const quotes_t & quotes )
			:	m_lock{ std::move(lock) }
			,	m_current_priority(
					&m_priorities[ to_size_t( so_5::priority_t::p_max ) ] )
			{
				so_5::prio::for_each_priority( [&]( priority_t p ) {
						auto & q = m_priorities[ to_size_t(p) ];
						// Every subqueue must have a valid pointer to main demand
						// queue.
						q.m_demand_queue = this;
						// Quote for the subqueue must be defined.
						q.m_quote = quotes.query( p );
					} );
			}
		~demand_queue_t()
			{
				for( auto & q : m_priorities )
					cleanup_queue( q );
			}

		//! Set the shutdown signal.
		void
		stop()
			{
				queue_traits::lock_guard_t lock{ *m_lock };

				m_shutdown = true;

				if( !m_total_demands_count )
					// There could be a sleeping working thread.
					// It must be notified.
					lock.notify_one();
			}

		//! Pop demand from the queue.
		/*!
		 * \throw shutdown_ex_t in the case when queue is shut down.
		 */
		demand_unique_ptr_t
		pop()
			{
				queue_traits::unique_lock_t lock{ *m_lock };

				while( !m_shutdown && !m_total_demands_count )
					lock.wait_for_notify();

				if( m_shutdown )
					throw shutdown_ex_t();

				// Note: this loop should not be infinitife because
				// m_total_demands_count is not a zero. It means that
				// there is at least one demand somewhere.
				while( !m_current_priority->m_head )
					switch_to_lower_priority();

				// There is a demand to extract.
				demand_unique_ptr_t result{ m_current_priority->m_head };

				m_current_priority->m_head = result->m_next;
				if( !m_current_priority->m_head )
					m_current_priority->m_tail = nullptr;

				result->m_next = nullptr;

				--(m_current_priority->m_demands_count);
				--m_total_demands_count;

				++(m_current_priority->m_demands_processed);

				if( m_current_priority->m_demands_processed >=
						m_current_priority->m_quote )
					{
						// Processing of this priority on the current
						// iteration is finished.
						switch_to_lower_priority();
					}

				return result;
			}

		//! Get queue for the priority specified.
		event_queue_t &
		event_queue_by_priority( priority_t priority )
			{
				return m_priorities[ to_size_t(priority) ];
			}

		//! Notification about attachment of yet another agent to the queue.
		void
		agent_bound( priority_t priority )
			{
				++(m_priorities[ to_size_t(priority) ].m_agents_count);
			}

		//! Notification about detachment of an agent from the queue.
		void
		agent_unbound( priority_t priority )
			{
				--(m_priorities[ to_size_t(priority) ].m_agents_count);
			}

		//! A special method for handling statistical data for
		//! every subqueue.
		template< class Lambda >
		void
		handle_stats_for_each_prio( Lambda handler )
			{
				so_5::prio::for_each_priority( [&]( so_5::priority_t p ) {
						const auto & subqueue = m_priorities[ to_size_t(p) ];
						handler( queue_stats_t{ p,
								subqueue.m_quote,
								subqueue.m_agents_count.load( std::memory_order_relaxed ),
								subqueue.m_demands_count.load( std::memory_order_relaxed ) } );
					} );
			}

	private :
		//! Queue lock.
		queue_traits::lock_unique_ptr_t m_lock;

		//! Shutdown flag.
		bool m_shutdown = false;

		//! Total count of demands in the queue.
		std::size_t m_total_demands_count = 0;

		//! Subqueues for priorities.
		queue_for_one_priority_t m_priorities[
				static_cast< std::size_t >( priority_t::p_max ) + 1 ];

		//! Pointer to the current subqueue.
		queue_for_one_priority_t * m_current_priority = nullptr;

		//! Destroy all demands in the queue specified.
		void
		cleanup_queue( queue_for_one_priority_t & queue_info )
			{
				auto h = queue_info.m_head;
				while( h )
					{
						demand_unique_ptr_t t{ h };
						h = h->m_next;
					}
			}

		//! Push a new demand to the queue.
		void
		push(
			//! Subqueue for the demand.
			queue_for_one_priority_t * subqueue,
			//! Demand to be pushed.
			demand_unique_ptr_t demand )
			{
				queue_traits::lock_guard_t lock{ *m_lock };

				add_demand_to_queue( *subqueue, std::move( demand ) );
				++m_total_demands_count;

				if( 1 == m_total_demands_count )
					// Queue was empty. A sleeping working thread must
					// be notified.
					lock.notify_one();
			}

		//! Add a new demand to the tail of the queue specified.
		void
		add_demand_to_queue(
			queue_for_one_priority_t & queue,
			demand_unique_ptr_t demand )
			{
				if( queue.m_tail )
					{
						// Queue is not empty. Tail will be modified.
						queue.m_tail->m_next = demand.release();
						queue.m_tail = queue.m_tail->m_next;
					}
				else
					{
						// Queue is empty. The whole description will be modified.
						queue.m_head = demand.release();
						queue.m_tail = queue.m_head;
					}

				++(queue.m_demands_count);
			}

		void
		switch_to_lower_priority()
			{
				// Iteration on the current priority is finished.
				// Count of processed demands must be started from zero.
				m_current_priority->m_demands_processed = 0;

				// Try to find next subqueue.
				if( m_current_priority > &m_priorities[ 0 ] )
					--m_current_priority;
				else
					// Start new iteration from the highest priority.
					m_current_priority = &m_priorities[
							to_size_t( so_5::priority_t::p_max ) ];
			}
	};

} /* namespace impl */

} /* namespace quoted_round_robin */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */

