/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Reusable common implementation for thread-pool-like dispatchers.
 *
 * \since v.5.5.4
 */

#pragma once

#include <so_5/event_queue.hpp>

#include <so_5/disp/reuse/actual_work_thread_factory_to_use.hpp>
#include <so_5/disp/reuse/queue_of_queues.hpp>
#include <so_5/disp/reuse/thread_pool_stats.hpp>

#include <so_5/details/rollback_on_exception.hpp>

#include <mutex>

namespace so_5 {

namespace disp {

namespace thread_pool {

namespace common_implementation {

namespace stats = so_5::stats;
namespace tp_stats = so_5::disp::reuse::thread_pool_stats;

//
// dispatcher_t
//
/*!
 * \brief Reusable common implementation for thread-pool-like dispatchers.
 *
 * \since v.5.5.4
 */
template<
	typename Work_Thread,
	typename Dispatcher_Queue,
	typename Params,
	typename Adaptations >
class dispatcher_t final
	:	public tp_stats::stats_supplier_t
	{
	private :
		//FIXME: document this!
		using agent_queue_t = typename Dispatcher_Queue::item_t;

		using agent_queue_ref_t = so_5::intrusive_ptr_t< agent_queue_t >;

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
				 *
				 * \since v.5.5.4
				 */
				tp_stats::queue_description_holder_ref_t m_queue_desc;

				cooperation_data_t(
					agent_queue_ref_t queue,
					std::size_t agents,
					const stats::prefix_t & data_source_name_prefix,
					coop_id_t coop_id )
					:	m_queue( std::move( queue ) )
					,	m_agents( agents )
					,	m_queue_desc(
							tp_stats::make_queue_desc_holder(
									data_source_name_prefix,
									coop_id,
									agents ) )
					{}

				/*!
				 * \brief Update queue information for run-time monitoring.
				 *
				 * \since v.5.5.4
				 */
				void
				update_queue_stats()
					{
						m_queue_desc->m_desc.m_agent_count = m_agents;
						m_queue_desc->m_desc.m_queue_size = m_queue->size();
					}
			};

		//! Map from cooperation name to the cooperation data.
		using cooperation_map_t = std::map< coop_id_t, cooperation_data_t >;

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
				 * \brief Description of that queue for run-time monitoring.
				 *
				 * \note This description is created only if agent
				 * uses individual FIFO.
				 *
				 * \since v.5.5.4
				 *
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
				 * \brief Does agent use cooperation FIFO?
				 *
				 * \since v.5.5.4
				 */
				[[nodiscard]]
				bool
				cooperation_fifo() const
					{
						return !m_queue_desc;
					}

				/*!
				 * \brief Update queue description with current information.
				 *
				 * \attention Must be called only if !cooperation_fifo().
				 *
				 * \since v.5.5.4
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
		template< typename Dispatcher_Params >
		dispatcher_t(
			environment_t & env,
			const so_5::disp::reuse::work_thread_factory_mixin_t< Dispatcher_Params >
				& disp_params,
			const std::string_view name_base,
			std::size_t thread_count,
			const so_5::disp::mpmc_queue_traits::queue_params_t & queue_params )
			:	m_queue{ queue_params, thread_count }
			,	m_thread_count( thread_count )
			,	m_data_source( stats_supplier() )
			{
				m_threads.reserve( thread_count );

				for( std::size_t i = 0; i != m_thread_count; ++i )
				{
					// Since v.5.7.3 an instance of the actual work thread
					// has to be acquired via factory.
					auto work_thread_holder = acquire_work_thread(
							disp_params, env );

					m_threads.emplace_back( std::unique_ptr< Work_Thread >(
								new Work_Thread{
										outliving_mutable(m_queue),
										std::move(work_thread_holder)
								} ) );
				}

				m_data_source.get().set_data_sources_name_base(
						Adaptations::dispatcher_type_name(),
						name_base,
						this );
			}

		void
		start( environment_t & env )
			{
				m_data_source.start(
						outliving_mutable(env.stats_repository()) );

				for( auto & t : m_threads )
					t->start();
			}

		void
		shutdown_then_wait() noexcept
			{
				m_queue.shutdown();

				for( auto & t : m_threads )
					t->join();

				m_data_source.stop();
			}

		//! Preallocate all necessary resources for a new agent.
		void
		preallocate_resources_for_agent(
			agent_t & agent,
			const Params & params )
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				if( Adaptations::is_individual_fifo( params ) )
					bind_agent_with_inidividual_fifo(
							agent_ref_t{ &agent }, params );
				else
					bind_agent_with_cooperation_fifo(
							agent_ref_t{ &agent }, params );
			}

		//! Undo preallocation of resources for a new agent.
		void
		undo_preallocation_for_agent(
			agent_t & agent ) noexcept
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				auto it = m_agents.find( &agent );
				if( it != m_agents.end() )
					{
						if( it->second.cooperation_fifo() )
							{
								auto it_coop = m_cooperations.find(
										agent.so_coop().id() );
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

		//! Get resources allocated for an agent.
		[[nodiscard]]
		event_queue_t *
		query_resources_for_agent( agent_t & agent ) noexcept
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				auto it = m_agents.find( &agent );
				if( it->second.cooperation_fifo() )
					return m_cooperations.find( agent.so_coop().id() )->
							second.m_queue.get();
				else
					return it->second.m_queue.get();
			}

		//! Unbind agent from the dispatcher.
		void
		unbind_agent( agent_t & agent ) noexcept
			{
				undo_preallocation_for_agent( agent );
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
		stats::manually_registered_source_holder_t< tp_stats::data_source_t >
				m_data_source;

		//! Creation event queue for an agent with individual FIFO.
		void
		bind_agent_with_inidividual_fifo(
			agent_ref_t agent,
			const Params & params )
			{
				auto queue = make_new_agent_queue( params );

				m_agents.emplace(
						agent.get(),
						agent_data_t{
								queue,
								m_data_source.get().prefix(),
								agent.get() } );
			}

		//! Creation event queue for an agent with individual FIFO.
		/*!
		 * If the data for the agent's cooperation is not created yet
		 * it will be created.
		 */
		void
		bind_agent_with_cooperation_fifo(
			agent_ref_t agent,
			const Params & params )
			{
				const auto id = agent->so_coop().id();

				auto it = m_cooperations.find( id );
				if( it == m_cooperations.end() )
					it = m_cooperations.emplace(
							id,
							cooperation_data_t(
									make_new_agent_queue( params ),
									1,
									m_data_source.get().prefix(),
									id ) )
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
			}

		//! Helper method for creating event queue for agents/cooperations.
		agent_queue_ref_t
		make_new_agent_queue(
			const Params & params )
			{
				return agent_queue_ref_t(
						new agent_queue_t{ outliving_mutable(m_queue), params } );
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
				std::lock_guard< std::mutex > lock{ m_lock };

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

