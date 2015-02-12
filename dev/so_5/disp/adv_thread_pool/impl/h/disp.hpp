/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief An implementation of advanced thread pool dispatcher.
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <memory>
#include <map>
#include <iostream>
#include <forward_list>

#include <so_5/h/spinlocks.hpp>
#include <so_5/h/atomic_refcounted.hpp>

#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/disp.hpp>

#include <so_5/disp/reuse/h/mpmc_ptr_queue.hpp>

#if 0
	#define SO_5__CHECK_INVARIANT__(what, data, file, line) \
	if( !(what) ) { \
		std::cerr << file << ":" << line << ": FAILED INVARIANT: " << #what << "; data: " << data << std::endl; \
		std::abort(); \
	}
	#define SO_5__CHECK_INVARIANT(what, data) SO_5__CHECK_INVARIANT__(what, data, __FILE__, __LINE__)
#else
	#define SO_5__CHECK_INVARIANT(what, data)
#endif


namespace so_5
{

namespace disp
{

namespace adv_thread_pool
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
		struct demand_t
			{
				//! Actual demand.
				so_5::rt::execution_demand_t m_demand;

				//! Next item in queue.
				demand_t * m_next;

				demand_t()
					:	m_next( nullptr )
					{}
				demand_t( so_5::rt::execution_demand_t && original )
					:	m_demand( std::move( original ) )
					,	m_next( nullptr )
					{}
			};

	public :
		static const unsigned int thread_safe_worker = 2;
		static const unsigned int not_thread_safe_worker = 1;

		//! Constructor.
		agent_queue_t(
			//! Dispatcher queue to work with.
			dispatcher_queue_t & disp_queue )
			:	m_disp_queue( disp_queue )
			,	m_tail( &m_head )
			,	m_active( false )
			,	m_workers( 0 )
			{}

		~agent_queue_t()
			{
				while( m_head.m_next )
					delete_head();
			}

		//! Access to the queue's lock.
		spinlock_t &
		lock()
			{
				return m_lock;
			}

		//! Push next demand to queue.
		virtual void
		push( so_5::rt::execution_demand_t demand )
			{
				bool need_schedule = false;
				{
					// Do memory allocation before spinlock locking.
					auto new_demand = new demand_t( std::move( demand ) );

					std::lock_guard< spinlock_t > lock( m_lock );

					m_tail->m_next = new_demand;
					m_tail = m_tail->m_next;

					if( m_head.m_next == m_tail )
						{
							// Queue was empty. Need to detect
							// necessity of queue activation.
							if( !m_active )
								if( !is_there_not_thread_safe_worker() )
								{
									need_schedule = true;
									m_active = true;
								}
						}

					SO_5__CHECK_INVARIANT( !empty(), this )
					SO_5__CHECK_INVARIANT( m_active || is_there_any_worker(), this )
					SO_5__CHECK_INVARIANT( !(need_schedule && !m_active), this )
				}

				if( need_schedule )
					m_disp_queue.schedule( this );
			}

		//! Get the information about the front demand.
		/*!
		 * \attention This method must be called only on non-empty queue.
		 */
		so_5::rt::execution_demand_t
		peek_front()
			{
				SO_5__CHECK_INVARIANT( !empty(), this )
				SO_5__CHECK_INVARIANT( m_active, this )

				m_active = false;

				return m_head.m_next->m_demand;
			}

		//! Remove the front demand.
		/*!
		 * \retval true queue must be activated.
		 * \retval false queue must not be activated.
		 */
		bool
		worker_started(
			//! Type of worker.
			//! Must be thread_safe_worker or not_thread_safe_worker.
			unsigned int type_of_worker )
			{
				SO_5__CHECK_INVARIANT( !empty(), this );
				SO_5__CHECK_INVARIANT( !m_active, this );

				delete_head();
				if( !m_head.m_next )
					m_tail = &m_head;

				m_workers += type_of_worker;

				// Queue must be activated only if queue is not empty
				// and current worker is a thread safe worker.
				m_active = ( !empty() &&
						thread_safe_worker == type_of_worker );

				return m_active;
			}

		//! Signal about finishing of worker of the specified type.
		/*!
		 * \retval true queue must be activated.
		 * \retval false queue must not be activated.
		 */
		bool
		worker_finished(
			//! Type of worker.
			//! Must be thread_safe_worker or not_thread_safe_worker.
			unsigned int type_of_worker )
			{
				m_workers -= type_of_worker;

				bool old_active = m_active;
				if( !m_active )
					m_active = !empty();

				SO_5__CHECK_INVARIANT( !(m_active && empty()), this )
				SO_5__CHECK_INVARIANT(
						!old_active || m_active, this );

				return old_active != m_active;
			}

		//! Check the presence of any worker at the moment.
		bool
		is_there_any_worker() const
			{
				return 0 != m_workers;
			}

		//! Check the presence of thread unsafe worker.
		bool
		is_there_not_thread_safe_worker() const
			{
				return 0 != (m_workers & not_thread_safe_worker );
			}

		//! Is empty queue?
		bool
		empty() const { return nullptr == m_head.m_next; }

		//! Is active queue?
		bool
		active() const { return m_active; }

	private :
		//! Dispatcher queue for scheduling processing of events from
		//! this queue.
		dispatcher_queue_t & m_disp_queue;

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

		//! Is this queue activated?
		/*!
		 * Queue is activated if it is scheduled to dispatcher queue.
		 */
		bool m_active;

		//! Count of active workers.
		unsigned int m_workers;

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
// agent_queue_ref_t
//
/*!
 * \since v.5.4.0
 * \brief A typedef of smart pointer for agent_queue.
 */
typedef so_5::intrusive_ptr_t< agent_queue_t > agent_queue_ref_t;

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

		//! Thread alarm for long waiting.
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
						// This guard is necessary to ensure that queue
						// will exist until processing of queue finished.
						agent_queue_ref_t agent_queue_guard( agent_queue );

						process_queue( *agent_queue );
					}
			}

		//! Processing of demands from agent queue.
		void
		process_queue( agent_queue_t & queue )
			{
				std::unique_lock< spinlock_t > lock( queue.lock() );

				auto demand = queue.peek_front();
				if( queue.is_there_not_thread_safe_worker() )
					// We can't process any demand until thread unsafe
					// worker is working.
					return;

				auto hint = demand.m_receiver->so_create_execution_hint( demand );

				bool need_schedule = true;
				if( !hint.is_thread_safe() )
				{
					if( queue.is_there_any_worker() )
						// We can't process not thread safe demand until
						// there are some other workers.
						return;
					else
						need_schedule = queue.worker_started(
								agent_queue_t::not_thread_safe_worker );
				}
				else
					// Threa-safe worker can be started.
					need_schedule = queue.worker_started(
							agent_queue_t::thread_safe_worker );

				SO_5__CHECK_INVARIANT( !(need_schedule && queue.empty()), &queue )
				SO_5__CHECK_INVARIANT(
						!need_schedule || hint.is_thread_safe(), &queue );
				SO_5__CHECK_INVARIANT( !need_schedule || queue.active(), &queue );

				// Next few actions must be done on unlocked queue.
				lock.unlock();

				if( need_schedule )
					m_disp_queue->schedule( &queue );

				// Processing of event.
				hint.exec( m_thread_id );

				// Next actions must be done on locked queue.
				lock.lock();

				need_schedule = queue.worker_finished(
						hint.is_thread_safe() ?
								agent_queue_t::thread_safe_worker :
								agent_queue_t::not_thread_safe_worker );

				SO_5__CHECK_INVARIANT(
						!need_schedule || queue.active(), &queue );

				lock.unlock();

				if( need_schedule )
					m_disp_queue->schedule( &queue );
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
				agent_queue_ref_t m_queue;

				//! Count of agents form that cooperation.
				/*!
				 * When this counter is zero then cooperation data
				 * must be destroyed.
				 */
				std::size_t m_agents;

				cooperation_data_t(
					agent_queue_ref_t queue,
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
				agent_queue_ref_t m_queue;

				//! Cooperation FIFO flag.
				/*!
				 * Set to 'true' if agent is used cooperation FIFO.
				 */
				bool m_cooperation_fifo;

				agent_data_t(
					agent_queue_ref_t queue,
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
										m_cooperations.erase( it_coop );
									}
							}

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
			const params_t & /*params*/ )
			{
				agent_queue_ref_t queue = make_new_agent_queue();

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
			const params_t & /*params*/ )
			{
				auto it = m_cooperations.find( agent->so_coop_name() );
				if( it == m_cooperations.end() )
					it = m_cooperations.emplace(
									agent->so_coop_name(),
									cooperation_data_t( make_new_agent_queue(), 1 ) )
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
		agent_queue_ref_t
		make_new_agent_queue()
			{
				return agent_queue_ref_t( new agent_queue_t( m_queue ) );
			}
	};

} /* namespace impl */

} /* namespace adv_thread_pool */

} /* namespace disp */

} /* namespace so_5 */

