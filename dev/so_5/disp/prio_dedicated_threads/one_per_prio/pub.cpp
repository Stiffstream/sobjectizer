/*
	SObjectizer 5.
*/

/*!
 * \since v.5.5.8
 * \file
 * \brief Functions for creating and binding of the dispatcher with
 * dedicated threads per priority.
 */

#include <so_5/disp/prio_dedicated_threads/one_per_prio/h/pub.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/rt/h/send_functions.hpp>

#include <so_5/details/h/invoke_noexcept_code.hpp>

#include <algorithm>

namespace so_5 {

namespace disp {

namespace prio_dedicated_threads {

namespace one_per_prio {

namespace impl {

namespace stats = so_5::stats;
using work_thread_t = so_5::disp::reuse::work_thread::work_thread_t;

//
// dispatcher_t
//
/*!
 * \since v.5.5.8
 * \brief An actual implementation of dispatcher with dedicated thread
 * for every priority.
 */
class dispatcher_t : public so_5::dispatcher_t
	{
	public:
		dispatcher_t( disp_params_t params )
			:	m_data_source{ self() }
			{
				m_threads.reserve( so_5::prio::total_priorities_count );
				so_5::prio::for_each_priority( [&]( so_5::priority_t ) {
						auto lock_factory = params.queue_params().lock_factory();

						std::unique_ptr< work_thread_t > t{
								new work_thread_t{ std::move(lock_factory) } };
						m_threads.push_back( std::move(t) );
					} );
			}

		//! \name Implementation of dispatcher methods.
		//! \{
		virtual void
		start( environment_t & env ) override
			{
				m_data_source.start( env.stats_repository() );

				so_5::details::do_with_rollback_on_exception(
						[this] { launch_work_threads(); },
						[this] { m_data_source.stop(); } );
			}

		virtual void
		shutdown() override
			{
				so_5::details::invoke_noexcept_code( [this] {
						for( auto & t : m_threads )
							t->shutdown();
					} );
			}

		virtual void
		wait() override
			{
				so_5::details::invoke_noexcept_code( [this] {
						for( auto & t : m_threads )
							t->wait();

						m_data_source.stop();
					} );
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override
			{
				m_data_source.set_data_sources_name_base( name_base );
			}
		//! \}

		/*!
		 * \since v.5.4.0
		 * \brief Get a binding information for an agent.
		 */
		event_queue_t *
		get_agent_binding( priority_t priority )
			{
				return m_threads[ to_size_t( priority ) ]->get_agent_binding();
			}

		//! Notification about binding of yet another agent.
		void
		agent_bound( priority_t priority )
			{
				m_agents_per_priority[ to_size_t(priority) ] += 1;
			}

		//! Notification about unbinding of an agent.
		void
		agent_unbound( priority_t priority )
			{
				m_agents_per_priority[ to_size_t(priority) ] -= 1;
			}

	private:
		friend class disp_data_source_t;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

		/*!
		 * \since v.5.5.8
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 */
		class disp_data_source_t : public stats::manually_registered_source_t
			{
				//! Dispatcher to work with.
				dispatcher_t & m_dispatcher;

				//! Basic prefix for data sources.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t( dispatcher_t & disp )
					:	m_dispatcher( disp )
					{}

				virtual void
				distribute( const mbox_t & mbox ) override
					{
						std::size_t agents_count = 0;

						so_5::prio::for_each_priority( [&]( priority_t p ) {
								auto agents = m_dispatcher.m_agents_per_priority[
										to_size_t(p) ].load( std::memory_order_acquire );

								agents_count += agents;

								distribute_value_for_work_thread(
										mbox,
										p,
										agents,
										*(m_dispatcher.m_threads[ to_size_t(p) ]) );
							} );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agents_count );
					}

				void
				set_data_sources_name_base(
					const std::string & name_base )
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								"pdt-opp",
								name_base,
								&m_dispatcher );
					}

			private:
				void
				distribute_value_for_work_thread(
					const mbox_t & mbox,
					priority_t priority,
					std::size_t agents_count,
					work_thread_t & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-p" << to_size_t(priority);

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::work_thread_queue_size(),
								wt.demands_count() );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::agent_count(),
								agents_count );
					}
			};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

		//! Data source for run-time monitoring.
		disp_data_source_t m_data_source;

		//! Working threads for every priority.
		std::vector< std::unique_ptr< work_thread_t > > m_threads;

		//! Counters for agent count for every priority.
		std::atomic< std::size_t > m_agents_per_priority[ so_5::prio::total_priorities_count ];

		/*!
		 * \brief Just a helper method for getting reference to itself.
		 */
		dispatcher_t &
		self()
			{
				return *this;
			}

		//! Start all working threads.
		void
		launch_work_threads()
			{
				using namespace std;
				using namespace so_5::details;
				using namespace so_5::prio;

				// This helper vector will be used for shutdown of
				// started threads in the case of an exception.
				work_thread_t * started_threads[ total_priorities_count ];
				// Initially all items must be NULL.
				fill( begin(started_threads), end(started_threads), nullptr );

				do_with_rollback_on_exception( [&] {
						for( std::size_t i = 0; i != total_priorities_count; ++i )
							{
								m_agents_per_priority[ i ].store( 0,
										std::memory_order_release );

								m_threads[ i ]->start();

								// Thread successfully started. Pointer to it
								// must be used on rollback.
								started_threads[ i ] = m_threads[ i ].get();
							}
					},
					[&] {
						invoke_noexcept_code( [&] {
							// Shutdown all started threads.
							for( auto t : started_threads )
								if( t )
									{
										t->shutdown();
										t->wait();
									}
								else
									// No more started threads.
									break;
						} );
					} );
			}
	};

//
// binding_actions_mixin_t
//
/*!
 * \since v.5.5.8
 * \brief Implementation of binding actions to be reused
 * in various binder implementation.
 */
class binding_actions_mixin_t
	{
	protected :
		disp_binding_activator_t
		do_bind(
			dispatcher_t & disp,
			agent_ref_t agent )
			{
				auto result = [agent, &disp]() {
					agent->so_bind_to_dispatcher(
							*(disp.get_agent_binding( agent->so_priority() )) );
				};

				// Dispatcher must know about yet another agent bound.
				disp.agent_bound( agent->so_priority() );

				return result;
			}

		void
		do_unbind(
			dispatcher_t & disp,
			agent_ref_t agent )
			{
				// Dispatcher must know about yet another agent unbound.
				disp.agent_unbound( agent->so_priority() );
			}
	};

//
// disp_binder_t
//
/*!
 * \since v.5.5.8
 * \brief Binder for public dispatcher.
 */
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		dispatcher_t, binding_actions_mixin_t >;

//
// private_dispatcher_binder_t
//

/*!
 * \since v.5.5.8
 * \brief A binder for the private %strictly_ordered dispatcher.
 */
using private_dispatcher_binder_t =
	so_5::disp::reuse::binder_for_private_disp_template_t<
		private_dispatcher_handle_t,
		dispatcher_t,
		binding_actions_mixin_t >;

//
// real_private_dispatcher_t
//
/*!
 * \since v.5.5.8
 * \brief A real implementation of private_dispatcher interface.
 */
class real_private_dispatcher_t : public private_dispatcher_t
	{
	public :
		/*!
		 * Constructor creates a dispatcher instance and launces it.
		 */
		real_private_dispatcher_t(
			//! SObjectizer Environment to work in.
			environment_t & env,
			//! Value for creating names of data sources for
			//! run-time monitoring.
			const std::string & data_sources_name_base,
			//! Parameters for the dispatcher.
			disp_params_t params )
			:	m_disp( new dispatcher_t{ std::move(params) } )
			{
				m_disp->set_data_sources_name_base( data_sources_name_base );
				m_disp->start( env );
			}

		/*!
		 * Destructors shuts an instance down and waits for it.
		 */
		~real_private_dispatcher_t()
			{
				m_disp->shutdown();
				m_disp->wait();
			}

		virtual disp_binder_unique_ptr_t
		binder() override
			{
				return disp_binder_unique_ptr_t(
						new private_dispatcher_binder_t(
								private_dispatcher_handle_t( this ), *m_disp ) );
			}

	private :
		std::unique_ptr< dispatcher_t > m_disp;
	};

} /* namespace impl */

//
// private_dispatcher_t
//

private_dispatcher_t::~private_dispatcher_t()
	{}

//
// create_disp
//
SO_5_FUNC dispatcher_unique_ptr_t
create_disp( disp_params_t params )
	{
		return dispatcher_unique_ptr_t(
				new impl::dispatcher_t{ std::move(params) } );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	environment_t & env,
	const std::string & data_sources_name_base,
	disp_params_t params )
	{
		return private_dispatcher_handle_t(
				new impl::real_private_dispatcher_t{
						env,
						data_sources_name_base,
						std::move(params) } );
	}

//
// create_disp_binder
//
SO_5_FUNC disp_binder_unique_ptr_t
create_disp_binder(
	const std::string & disp_name )
	{
		return disp_binder_unique_ptr_t( 
			new impl::disp_binder_t( disp_name ) );
	}

} /* namespace one_per_prio */

} /* namespace prio_dedicated_threads */

} /* namespace disp */

} /* namespace so_5 */

