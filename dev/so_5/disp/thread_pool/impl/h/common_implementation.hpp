/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief Reusable common implementation for thread-pool-like dispatchers.
 */

#pragma once

#include <so_5/event_queue.hpp>
#include <so_5/disp.hpp>

#include <so_5/disp/reuse/h/mpmc_ptr_queue.hpp>
#include <so_5/disp/reuse/h/thread_pool_stats.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

#include <mutex>

namespace so_5 {

namespace disp {

namespace thread_pool {

namespace common_implementation {

namespace stats = so_5::stats;
namespace tp_stats = so_5::disp::reuse::thread_pool_stats;

//
// ext_dispatcher_iface_t
//
/*!
 * \brief Type of extended dispatcher interface.
 *
 * This interface introduces dispatcher specific bind_agent and
 * unbind_agent methods.
 *
 * \since
 * v.5.5.18
 */
template< typename Params >
class ext_dispatcher_iface_t : public so_5::dispatcher_t
	{
	public :
		//! Bind agent to the dispatcher.
		virtual event_queue_t *
		bind_agent( agent_ref_t agent, const Params & params ) = 0;

		//! Unbind agent from the dispatcher.
		virtual void
		unbind_agent( agent_ref_t agent ) = 0;
	};

//
// dispatcher_t
//
/*!
 * \brief Reusable common implementation for thread-pool-like dispatchers.
 * \since
 * v.5.5.4
 */
template<
	typename Work_Thread,
	typename Dispatcher_Queue,
	typename Agent_Queue,
	typename Params,
	typename Adaptations >
class dispatcher_t
	:	public ext_dispatcher_iface_t< Params >
	,	public tp_stats::stats_supplier_t
	{
	private :
		using agent_queue_ref_t = so_5::intrusive_ptr_t< Agent_Queue >;

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

				/*!
				 * \brief Description of that queue for run-time monitoring.
				 * \since
				 * v.5.5.4
				 */
				tp_stats::queue_description_holder_ref_t m_queue_desc;

				cooperation_data_t(
					agent_queue_ref_t queue,
					std::size_t agents,
					const stats::prefix_t & data_source_name_prefix,
					const std::string & coop_name )
					:	m_queue( std::move( queue ) )
					,	m_agents( agents )
					,	m_queue_desc(
							tp_stats::make_queue_desc_holder(
									data_source_name_prefix,
									coop_name,
									agents ) )
					{}

				/*!
				 * \since
				 * v.5.5.4
				 *
				 * \brief Update queue information for run-time monitoring.
				 */
				void
				update_queue_stats()
					{
						m_queue_desc->m_desc.m_agent_count = m_agents;
						m_queue_desc->m_desc.m_queue_size = m_queue->size();
					}
			};

		//! Map from cooperation name to the cooperation data.
		using cooperation_map_t = std::map< std::string, cooperation_data_t >;

		//! Data for one agent.
		struct agent_data_t
			{
				//! Event queue for the agent.
				/*!
				 * It could be individual queue or queue for the whole
				 * cooperation (to which agent is belonging).
				 */
				agent_queue_ref_t m_queue;

				/*!
				 * \since
				 * v.5.5.4
				 *
				 * \brief Description of that queue for run-time monitoring.
				 *
				 * \note This description is created only if agent
				 * uses individual FIFO.
				 */
				tp_stats::queue_description_holder_ref_t m_queue_desc;

				//! Constructor for the case when agent uses cooperation FIFO.
				agent_data_t(
					agent_queue_ref_t queue )
					:	m_queue( std::move( queue ) )
					{}

				//! Constructor for the case when agent uses individual FIFO.
				/*!
				 * In this case a queue_description object must be created.
				 */
				agent_data_t(
					agent_queue_ref_t queue,
					const stats::prefix_t & data_source_name_prefix,
					const agent_t * agent_ptr )
					:	m_queue( std::move( queue ) )
					,	m_queue_desc(
							tp_stats::make_queue_desc_holder(
									data_source_name_prefix,
									agent_ptr ) )
					{}

				/*!
				 * \since
				 * v.5.5.4
				 *
				 * \brief Does agent use cooperation FIFO?
				 */
				bool
				cooperation_fifo() const
					{
						return !m_queue_desc;
					}

				/*!
				 * \since
				 * v.5.5.4
				 *
				 * \brief Update queue description with current information.
				 *
				 * \attention Must be called only if !cooperation_fifo().
				 */
				void
				update_queue_stats()
					{
						m_queue_desc->m_desc.m_agent_count = 1;
						m_queue_desc->m_desc.m_queue_size = m_queue->size();
					}
			};

		//! Map from agent pointer to the agent data.
		using agent_map_t = std::map< agent_t *, agent_data_t >;

	public :
		dispatcher_t( const dispatcher_t & ) = delete;
		dispatcher_t & operator=( const dispatcher_t & ) = delete;

		//! Constructor.
		dispatcher_t(
			std::size_t thread_count,
			const so_5::disp::mpmc_queue_traits::queue_params_t & queue_params )
			:	m_queue{ queue_params, thread_count }
			,	m_thread_count( thread_count )
			,	m_data_source( stats_supplier() )
			{
				m_threads.reserve( thread_count );

				for( std::size_t i = 0; i != m_thread_count; ++i )
					m_threads.emplace_back( std::unique_ptr< Work_Thread >(
								new Work_Thread( m_queue ) ) );
			}

		virtual void
		start( environment_t & env ) override
			{
				m_data_source.start( outliving_mutable(env.stats_repository()) );

				for( auto & t : m_threads )
					t->start();
			}

		virtual void
		shutdown() override
			{
				m_queue.shutdown();
			}

		virtual void
		wait() override
			{
				for( auto & t : m_threads )
					t->join();

				m_data_source.stop();
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override
			{
				m_data_source.set_data_sources_name_base(
						Adaptations::dispatcher_type_name(),
						name_base,
						this );
			}

		//! Bind agent to the dispatcher.
		virtual event_queue_t *
		bind_agent( agent_ref_t agent, const Params & params ) override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				if( Adaptations::is_individual_fifo( params ) )
					return bind_agent_with_inidividual_fifo(
							std::move( agent ), params );

				return bind_agent_with_cooperation_fifo(
						std::move( agent ), params );
			}

		//! Unbind agent from the dispatcher.
		virtual void
		unbind_agent( agent_ref_t agent ) override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				auto it = m_agents.find( agent.get() );
				if( it != m_agents.end() )
					{
						if( it->second.cooperation_fifo() )
							{
								auto it_coop = m_cooperations.find(
										agent->so_coop_name() );
								if( it_coop != m_cooperations.end() &&
										0 == --(it_coop->second.m_agents) )
									{
										// agent_queue object can be destroyed
										// only when it is empty.
										Adaptations::wait_for_queue_emptyness(
												*(it_coop->second.m_queue) );

										m_cooperations.erase( it_coop );
									}
							}
						else
							// agent_queue object can be destroyed
							// only when it is empty.
							Adaptations::wait_for_queue_emptyness(
									*(it->second.m_queue) );

						m_agents.erase( it );
					}
			}

	private :
		//! Queue for active agent's queues.
		Dispatcher_Queue m_queue;

		//! Count of working threads.
		const std::size_t m_thread_count;

		//! Pool of work threads.
		std::vector< std::unique_ptr< Work_Thread > > m_threads;

		//! Object's lock.
		std::mutex m_lock;

		//! Information about cooperations.
		/*!
		 * Information to this map is added only if an agent is
		 * using cooperation FIFO mechanism.
		 */
		cooperation_map_t m_cooperations;

		//! Information of agents.
		agent_map_t m_agents;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Data source for the run-time monitoring.
		 */
		tp_stats::data_source_t m_data_source;

		//! Creation event queue for an agent with individual FIFO.
		event_queue_t *
		bind_agent_with_inidividual_fifo(
			agent_ref_t agent,
			const Params & params )
			{
				auto queue = make_new_agent_queue( params );

				m_agents.emplace(
						agent.get(),
						agent_data_t{
								queue,
								m_data_source.prefix(),
								agent.get() } );

				return queue.get();
			}

		//! Creation event queue for an agent with individual FIFO.
		/*!
		 * If the data for the agent's cooperation is not created yet
		 * it will be created.
		 */
		event_queue_t *
		bind_agent_with_cooperation_fifo(
			agent_ref_t agent,
			const Params & params )
			{
				auto it = m_cooperations.find( agent->so_coop_name() );
				if( it == m_cooperations.end() )
					it = m_cooperations.emplace(
							agent->so_coop_name(),
							cooperation_data_t(
									make_new_agent_queue( params ),
									1,
									m_data_source.prefix(),
									agent->so_coop_name() ) )
							.first;
				else
					it->second.m_agents += 1;

				so_5::details::do_with_rollback_on_exception(
						[&] {
							m_agents.emplace(
									agent.get(),
									agent_data_t{ it->second.m_queue } );
						},
						[&] {
							// Rollback m_cooperations modification.
							if( 0 == --(it->second.m_agents) )
								m_cooperations.erase( it );
						} );

				return it->second.m_queue.get();
			}

		//! Helper method for creating event queue for agents/cooperations.
		agent_queue_ref_t
		make_new_agent_queue(
			const Params & params )
			{
				return agent_queue_ref_t(
						new Agent_Queue{ m_queue, params } );
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Helper method for casting to stats_supplier-object.
		 */
		tp_stats::stats_supplier_t &
		stats_supplier()
			{
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Implementation of stats_supplier-related stuff.
		 */
		virtual void
		supply( tp_stats::stats_consumer_t & consumer ) override
			{
				// Statics must be collected on locked object.
				std::lock_guard< std::mutex > lock( m_lock );

				consumer.set_thread_count( m_threads.size() );

				for( auto & t : m_threads )
					{
						using stats_t = so_5::stats::work_thread_activity_stats_t;

						Work_Thread & wt = *t;
						wt.take_activity_stats(
							[&wt, &consumer]( const stats_t & st ) {
								consumer.add_work_thread_activity( wt.thread_id(), st );
							} );
					}

				for( auto & q : m_cooperations )
					{
						auto & s = q.second;
						s.update_queue_stats();
						consumer.add_queue( s.m_queue_desc );
					}

				for( auto & a : m_agents )
					{
						auto & s = a.second;
						if( !s.cooperation_fifo() )
							{
								s.update_queue_stats();
								consumer.add_queue( s.m_queue_desc );
							}
					}
			}
	};

} /* namespace common_implementation */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */



