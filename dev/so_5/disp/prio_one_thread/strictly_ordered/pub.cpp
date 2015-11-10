/*
	SObjectizer 5.
*/

/*!
 * \since v.5.5.8
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * with priority support.
 */

#include <so_5/disp/prio_one_thread/strictly_ordered/h/pub.hpp>

#include <so_5/disp/prio_one_thread/strictly_ordered/impl/h/demand_queue.hpp>
#include <so_5/disp/prio_one_thread/reuse/h/work_thread.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/rt/h/send_functions.hpp>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace strictly_ordered {

namespace impl {

namespace stats = so_5::rt::stats;

//
// dispatcher_t
//
/*!
 * \since v.5.5.8
 * \brief An actual implementation of dispatcher with one working
 * thread and support of demand priorities.
 */
class dispatcher_t : public so_5::rt::dispatcher_t
	{
	public:
		dispatcher_t( disp_params_t params )
			:	m_demand_queue{ params.queue_params().lock_factory()() }
			,	m_work_thread{ m_demand_queue }
			,	m_data_source{ self() }
			{}

		//! \name Implementation of so_5::rt::dispatcher methods.
		//! \{
		virtual void
		start( so_5::rt::environment_t & env ) override
			{
				m_data_source.start( env.stats_repository() );

				so_5::details::do_with_rollback_on_exception(
						[this] { m_work_thread.start(); },
						[this] { m_data_source.stop(); } );
			}

		virtual void
		shutdown() override
			{
				m_demand_queue.stop();
			}

		virtual void
		wait() override
			{
				m_work_thread.join();

				m_data_source.stop();
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
		so_5::rt::event_queue_t *
		get_agent_binding( priority_t priority )
			{
				return &m_demand_queue.event_queue_by_priority( priority );
			}

		//! Notification about binding of yet another agent.
		void
		agent_bound( priority_t priority )
			{
				m_demand_queue.agent_bound( priority );
			}

		//! Notification about unbinding of an agent.
		void
		agent_unbound( priority_t priority )
			{
				m_demand_queue.agent_unbound( priority );
			}

	private:
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
				distribute( const so_5::rt::mbox_t & mbox ) override
					{
						std::size_t agents_count = 0;

						m_dispatcher.m_demand_queue.handle_stats_for_each_prio(
							[&]( const demand_queue_t::queue_stats_t & stat ) {
								distribute_value_for_priority(
									mbox,
									stat.m_priority,
									stat.m_agents_count,
									stat.m_demands_count );

								agents_count += stat.m_agents_count;
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
								"pot-so",
								name_base,
								&m_dispatcher );
					}

			private:
				void
				distribute_value_for_priority(
					const so_5::rt::mbox_t & mbox,
					priority_t priority,
					std::size_t agents_count,
					std::size_t demands_count )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/p" << to_size_t(priority);

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::agent_count(),
								agents_count );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::work_thread_queue_size(),
								demands_count );
					}
			};

		//! Demand queue for the dispatcher.
		demand_queue_t m_demand_queue;

		//! Working thread for the dispatcher.
		so_5::disp::prio_one_thread::reuse::work_thread_t< demand_queue_t >
				m_work_thread;

		//! Data source for run-time monitoring.
		disp_data_source_t m_data_source;

		/*!
		 * \brief Just a helper method for getting reference to itself.
		 */
		dispatcher_t &
		self()
			{
				return *this;
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
		so_5::rt::disp_binding_activator_t
		do_bind(
			dispatcher_t & disp,
			so_5::rt::agent_ref_t agent )
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
			so_5::rt::agent_ref_t agent )
			{
				// Dispatcher must know about yet another agent bound.
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
			so_5::rt::environment_t & env,
			//! Value for creating names of data sources for
			//! run-time monitoring.
			const std::string & data_sources_name_base,
			//! Parameters for the dispatcher.
			disp_params_t params )
			:	m_disp( new dispatcher_t( std::move(params) ) )
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

		virtual so_5::rt::disp_binder_unique_ptr_t
		binder() override
			{
				return so_5::rt::disp_binder_unique_ptr_t(
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
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp( disp_params_t params )
	{
		return so_5::rt::dispatcher_unique_ptr_t(
				new impl::dispatcher_t( std::move(params) ) );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	so_5::rt::environment_t & env,
	const std::string & data_sources_name_base,
	disp_params_t params )
	{
		return private_dispatcher_handle_t(
				new impl::real_private_dispatcher_t(
						env,
						data_sources_name_base,
						std::move(params) ) );
	}

//
// create_disp_binder
//
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	const std::string & disp_name )
	{
		return so_5::rt::disp_binder_unique_ptr_t( 
			new impl::disp_binder_t( disp_name ) );
	}

} /* namespace strictly_ordered */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */

