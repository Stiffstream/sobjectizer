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

#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/disp.hpp>

#include <so_5/disp/reuse/h/mpmc_ptr_queue.hpp>

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
class agent_queue_t : public so_5::rt::event_queue_t
	{
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
			//! Maximum count of demands to be processed at once.
			std::size_t max_demands_at_once )
			:	m_disp_queue( disp_queue )
			,	m_max_demands_at_once( max_demands_at_once )
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
				std::lock_guard< spinlock_t > lock( m_lock );

				bool was_empty = (nullptr == m_head.m_next);

				m_tail->m_next = new demand_t( std::move( demand ) );
				m_tail = m_tail->m_next;

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

		//! Helper method for deleting queue's head object.
		inline void
		delete_head()
			{
				auto to_be_deleted = m_head.m_next;
				m_head.m_next = m_head.m_next->m_next;

				delete to_be_deleted;
			}
	};

//
// agent_queue_shptr_t
//
/*!
 * \since v.5.4.0
 * \brief A typedef of std::shared_ptr for agent_queue.
 */
typedef std::shared_ptr< agent_queue_t > agent_queue_shptr_t;

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
		/*!
		 * Automatically starts work thread.
		 */
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

						(*d.m_demand_handler)( m_thread_id, d );

						++demands_processed;
						need_continue = queue.pop( demands_processed );
					}
			}
	};

//
// dispatcher_t
//
/*!
 * \since v.5.4.0
 * \brief An implementation of thread pool dispatcher.
 */
class dispatcher_t : public so_5::rt::dispatcher_t
	{
	private :
		//! Data for one cooperation.
		struct cooperation_data_t
			{
				//! Event queue for the cooperation.
				agent_queue_shptr_t m_queue;

				//! Count of agents form that cooperation.
				/*!
				 * When this counter is zero then cooperation data
				 * must be destroyed.
				 */
				std::size_t m_agents;

				cooperation_data_t(
					agent_queue_shptr_t queue,
					std::size_t agents )
					:	m_queue( std::move( queue ) )
					,	m_agents( agents )
					{}
			};

		//! Map from cooperation name to the cooperation data.
		typedef std::map< std::string, cooperation_data_t >
				cooperation_map_t;

		//! Data for one agent.
		struct agent_data_t
			{
				//! Event queue for the agent.
				/*!
				 * It could be individual queue or queue for the whole
				 * cooperation (to which agent is belonging).
				 */
				agent_queue_shptr_t m_queue;

				//! Cooperation FIFO flag.
				/*!
				 * Set to 'true' if agent is used cooperation FIFO.
				 */
				bool m_cooperation_fifo;

				agent_data_t(
					agent_queue_shptr_t queue,
					bool cooperation_fifo )
					:	m_queue( std::move( queue ) )
					,	m_cooperation_fifo( cooperation_fifo )
					{}
			};

		//! Map from agent pointer to the agent data.
		typedef std::map< so_5::rt::agent_t *, agent_data_t >
				agent_map_t;

	public :
		dispatcher_t( const dispatcher_t & ) = delete;
		dispatcher_t & operator=( const dispatcher_t & ) = delete;

		//! Constructor.
		dispatcher_t(
			std::size_t thread_count )
			:	m_thread_count( thread_count )
			{
				m_threads.reserve( thread_count );

				for( std::size_t i = 0; i != m_thread_count; ++i )
					m_threads.emplace_back( std::unique_ptr< work_thread_t >(
								new work_thread_t( m_queue ) ) );
			}

		virtual void
		start()
			{
				for( auto & t : m_threads )
					t->start();
			}

		virtual void
		shutdown()
			{
				m_queue.shutdown();
			}

		virtual void
		wait()
			{
				for( auto & t : m_threads )
					t->join();
			}

		//! Bind agent to the dispatcher.
		so_5::rt::event_queue_t *
		bind_agent(
			so_5::rt::agent_ref_t agent,
			const params_t & params )
			{
				std::lock_guard< spinlock_t > lock( m_lock );

				if( fifo_t::individual == params.query_fifo() )
					return bind_agent_with_inidividual_fifo(
							std::move( agent ), params );

				return bind_agent_with_cooperation_fifo(
						std::move( agent ), params );
			}

		//! Unbind agent from the dispatcher.
		void
		unbind_agent(
			so_5::rt::agent_ref_t agent )
			{
				std::lock_guard< spinlock_t > lock( m_lock );

				auto it = m_agents.find( agent.get() );
				if( it != m_agents.end() )
					{
						if( it->second.m_cooperation_fifo )
							{
								auto it_coop = m_cooperations.find(
										agent->so_coop_name() );
								if( it_coop != m_cooperations.end() &&
										0 == --(it_coop->second.m_agents) )
									{
										// agent_queue object can be destroyed
										// only when it is empty.
										it_coop->second.m_queue->wait_for_emptyness();

										m_cooperations.erase( it_coop );
									}
							}
						else
							// agent_queue object can be destroyed
							// only when it is empty.
							it->second.m_queue->wait_for_emptyness();

						m_agents.erase( it );
					}
			}

	private :
		//! Queue for active agent's queues.
		dispatcher_queue_t m_queue;

		//! Count of working threads.
		const std::size_t m_thread_count;

		//! Pool of work threads.
		std::vector< std::unique_ptr< work_thread_t > > m_threads;

		//! Object's lock.
		spinlock_t m_lock;

		//! Information about cooperations.
		/*!
		 * Information to this map is added only if an agent is
		 * using cooperation FIFO mechanism.
		 */
		cooperation_map_t m_cooperations;

		//! Information of agents.
		agent_map_t m_agents;

		//! Creation event queue for an agent with individual FIFO.
		so_5::rt::event_queue_t *
		bind_agent_with_inidividual_fifo(
			so_5::rt::agent_ref_t agent,
			const params_t & params )
			{
				agent_queue_shptr_t queue = make_new_agent_queue( params );

				m_agents.emplace( agent.get(),
						agent_data_t( queue, false ) );

				return queue.get();
			}

		//! Creation event queue for an agent with individual FIFO.
		/*!
		 * If the data for the agent's cooperation is not created yet
		 * it will be created.
		 */
		so_5::rt::event_queue_t *
		bind_agent_with_cooperation_fifo(
			so_5::rt::agent_ref_t agent,
			const params_t & params )
			{
				auto it = m_cooperations.find( agent->so_coop_name() );
				if( it == m_cooperations.end() )
					it = m_cooperations.emplace(
							agent->so_coop_name(),
							cooperation_data_t(
									make_new_agent_queue( params ),
									1 ) )
							.first;
				else
					it->second.m_agents += 1;

				try
					{
						m_agents.emplace( agent.get(),
								agent_data_t( it->second.m_queue, true ) );
					}
				catch( ... )
					{
						// Rollback m_cooperations modification.
						if( 0 == --(it->second.m_agents) )
							m_cooperations.erase( it );
						throw;
					}

				return it->second.m_queue.get();

			}

		//! Helper method for creating event queue for agents/cooperations.
		agent_queue_shptr_t
		make_new_agent_queue(
			const params_t & params )
			{
				return agent_queue_shptr_t(
						new agent_queue_t(
								m_queue,
								params.query_max_demands_at_once() ) );
			}
	};

} /* namespace impl */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */

