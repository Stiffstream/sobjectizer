/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * with priority support (quoted round robin policy).
 *
 * \since
 * v.5.5.8
 */

#include <so_5/disp/prio_one_thread/quoted_round_robin/h/pub.hpp>

#include <so_5/disp/prio_one_thread/quoted_round_robin/impl/h/demand_queue.hpp>
#include <so_5/disp/prio_one_thread/reuse/h/work_thread.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/h/proxy_dispatcher_template.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/rt/send_functions.hpp>

#include <so_5/stdcpp.hpp>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace quoted_round_robin {

namespace impl {

namespace stats = so_5::stats;

//
// actual_disp_iface_t
//
/*!
 * \brief An actual interface of quoted round-robin priority dispatcher.
 *
 * \since
 * v.5.5.18
 */
class actual_disp_iface_t : public so_5::dispatcher_t
	{
	public :
		//! Create binding for an agent with given priority.
		virtual event_queue_t *
		get_agent_binding( priority_t priority ) = 0;

		//! Notification about binding of yet another agent.
		virtual void
		agent_bound( priority_t priority ) = 0;

		//! Notification about unbinding of an agent.
		virtual void
		agent_unbound( priority_t priority ) = 0;
	};

namespace {

void
send_thread_activity_stats(
	const so_5::mbox_t &,
	const stats::prefix_t &,
	so_5::disp::prio_one_thread::reuse::work_thread_no_activity_tracking_t<
			demand_queue_t > & )
	{
		/* Nothing to do */
	}

void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	so_5::disp::prio_one_thread::reuse::work_thread_with_activity_tracking_t<
			demand_queue_t > & wt )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				prefix,
				stats::suffixes::work_thread_activity(),
				wt.thread_id(),
				wt.take_activity_stats() );
	}

} /* namespace anonymous */

//
// dispatcher_template_t
//
/*!
 * \brief An implementation of dispatcher with one working thread and support
 * of demand priorities (quoted round robin policy) in form of template class.
 *
 * \since
 * v.5.5.8, v.5.5.18
 */
template< typename Work_Thread >
class dispatcher_template_t : public actual_disp_iface_t
	{
		friend class disp_data_source_t;

	public:
		dispatcher_template_t(
			const quotes_t & quotes,
			disp_params_t params )
			:	m_demand_queue{
					params.queue_params().lock_factory()(),
					quotes }
			,	m_work_thread{ m_demand_queue }
			,	m_data_source{ self() }
			{}

		virtual void
		start( environment_t & env ) override
			{
				m_data_source.start( outliving_mutable(env.stats_repository()) );

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

		virtual event_queue_t *
		get_agent_binding( priority_t priority ) override
			{
				return &m_demand_queue.event_queue_by_priority( priority );
			}

		virtual void
		agent_bound( priority_t priority ) override
			{
				m_demand_queue.agent_bound( priority );
			}

		virtual void
		agent_unbound( priority_t priority ) override
			{
				m_demand_queue.agent_unbound( priority );
			}

	private:

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since
		 * v.5.5.8
		 */
		class disp_data_source_t : public stats::manually_registered_source_t
			{
				//! Dispatcher to work with.
				dispatcher_template_t & m_dispatcher;

				//! Basic prefix for data sources.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t( dispatcher_template_t & disp )
					:	m_dispatcher( disp )
					{}

				void
				distribute( const mbox_t & mbox ) override
					{
						std::size_t agents_count = 0;

						m_dispatcher.m_demand_queue.handle_stats_for_each_prio(
							[&]( const demand_queue_t::queue_stats_t & stat ) {
								distribute_value_for_priority(
									mbox,
									stat.m_priority,
									stat.m_quote,
									stat.m_agents_count,
									stat.m_demands_count );

								agents_count += stat.m_agents_count;
							} );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agents_count );

						send_thread_activity_stats(
								mbox,
								m_base_prefix,
								m_dispatcher.m_work_thread );
					}

				void
				set_data_sources_name_base(
					const std::string & name_base )
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								"pot-qrr",
								name_base,
								&m_dispatcher );
					}

			private:
				void
				distribute_value_for_priority(
					const mbox_t & mbox,
					priority_t priority,
					std::size_t quote,
					std::size_t agents_count,
					std::size_t demands_count )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/p" << to_size_t(priority);

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::demand_quote(),
								quote );

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
		Work_Thread m_work_thread;

		//! Data source for run-time monitoring.
		disp_data_source_t m_data_source;

		/*!
		 * \brief Just a helper method for getting reference to itself.
		 */
		dispatcher_template_t &
		self()
			{
				return *this;
			}
	};

//
// proxy_dispatcher_t
//

using proxy_dispatcher_base_t =
		so_5::disp::reuse::proxy_dispatcher_template_t<
				actual_disp_iface_t,
				disp_params_t >;

/*!
 * \brief A proxy dispatcher which creates actual dispatcher at start.
 *
 * \since
 * v.5.5.18
 *
 * This proxy is necessary because named dispatchers which are created
 * by create_disp() functions do not have a reference to SObjectizer
 * Environment at creation time. That reference is available in start()
 * method. Because of that creation of actual dispatcher (with or without
 * activity tracking) is delayed and performed only in start() method.
 */
class proxy_dispatcher_t : public proxy_dispatcher_base_t
	{
	public:
		proxy_dispatcher_t(
			const quotes_t & quotes,
			disp_params_t params )
			:	proxy_dispatcher_base_t( std::move(params) )
			,	m_quotes{ quotes }
			{}

		virtual event_queue_t *
		get_agent_binding( priority_t priority ) override
			{
				return m_disp->get_agent_binding( priority );
			}

		virtual void
		agent_bound( priority_t priority ) override
			{
				m_disp->agent_bound( priority );
			}

		virtual void
		agent_unbound( priority_t priority ) override
			{
				m_disp->agent_unbound( priority );
			}

	protected :
		virtual void
		do_actual_start( environment_t & env ) override
			{
				using namespace so_5::disp::prio_one_thread::reuse;

				using dispatcher_no_activity_tracking_t =
						dispatcher_template_t<
							work_thread_no_activity_tracking_t<
								demand_queue_t > >;

				using dispatcher_with_activity_tracking_t =
						dispatcher_template_t<
							work_thread_with_activity_tracking_t<
								demand_queue_t > >;

				make_actual_dispatcher<
							dispatcher_no_activity_tracking_t,
							dispatcher_with_activity_tracking_t >(
						env,
						m_quotes,
						m_disp_params );
			}

	private :
		const quotes_t m_quotes;
	};
//
// binding_actions_mixin_t
//
/*!
 * \brief Implementation of binding actions to be reused
 * in various binder implementation.
 *
 * \since
 * v.5.5.8
 */
class binding_actions_mixin_t
	{
	protected :
		disp_binding_activator_t
		do_bind(
			actual_disp_iface_t & disp,
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
			actual_disp_iface_t & disp,
			agent_ref_t agent )
			{
				// Dispatcher must know about yet another agent bound.
				disp.agent_unbound( agent->so_priority() );
			}
	};

//
// disp_binder_t
//
/*!
 * \brief Binder for public dispatcher.
 *
 * \since
 * v.5.5.8
 */
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		proxy_dispatcher_t, binding_actions_mixin_t >;

//
// private_dispatcher_binder_t
//

/*!
 * \brief A binder for the private %strictly_ordered dispatcher.
 *
 * \since
 * v.5.5.8
 */
using private_dispatcher_binder_t =
	so_5::disp::reuse::binder_for_private_disp_template_t<
		private_dispatcher_handle_t,
		proxy_dispatcher_t,
		binding_actions_mixin_t >;

//
// real_private_dispatcher_t
//
/*!
 * \brief A real implementation of private_dispatcher interface.
 *
 * \since
 * v.5.5.8
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
			//! Quotes for every priority.
			const quotes_t & quotes,
			//! Value for creating names of data sources for
			//! run-time monitoring.
			const std::string & data_sources_name_base,
			//! Parameters for the dispatcher.
			disp_params_t params )
			:	m_disp{ so_5::stdcpp::make_unique< proxy_dispatcher_t >(
					quotes, std::move(params) ) }
			{
				m_disp->set_data_sources_name_base( data_sources_name_base );
				m_disp->start( env );
			}

		/*!
		 * Destructors shuts an instance down and waits for it.
		 */
		~real_private_dispatcher_t() override
			{
				m_disp->shutdown();
				m_disp->wait();
			}

		virtual disp_binder_unique_ptr_t
		binder() override
			{
				return so_5::stdcpp::make_unique< private_dispatcher_binder_t >(
						private_dispatcher_handle_t( this ),
						*m_disp );
			}

	private :
		std::unique_ptr< proxy_dispatcher_t > m_disp;
	};

} /* namespace impl */

//
// create_disp
//
SO_5_FUNC dispatcher_unique_ptr_t
create_disp( const quotes_t & quotes, disp_params_t params )
	{
		return so_5::stdcpp::make_unique< impl::proxy_dispatcher_t >(
				quotes,
				std::move(params) );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	environment_t & env,
	const quotes_t & quotes,
	const std::string & data_sources_name_base,
	disp_params_t params )
	{
		return private_dispatcher_handle_t{
				new impl::real_private_dispatcher_t{
						env,
						quotes,
						data_sources_name_base,
						std::move(params)
				}
			};
	}

//
// create_disp_binder
//
SO_5_FUNC disp_binder_unique_ptr_t
create_disp_binder(
	const std::string & disp_name )
	{
		return so_5::stdcpp::make_unique< impl::disp_binder_t >( disp_name );
	}

} /* namespace quoted_round_robin */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */

