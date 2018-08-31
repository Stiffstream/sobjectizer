/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/h/pub.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/environment.hpp>
#include <so_5/rt/h/send_functions.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/rt/stats/impl/h/activity_tracking.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/h/proxy_dispatcher_template.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

#include <so_5/h/stdcpp.hpp>

#include <atomic>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::stats;

namespace data_source_details
{

template< typename Work_Thread >
struct common_data_t
	{
		//! Prefix for dispatcher-related data.
		stats::prefix_t m_base_prefix;
		//! Prefix for working thread-related data.
		stats::prefix_t m_work_thread_prefix;

		//! Working thread of the dispatcher.
		Work_Thread & m_work_thread;

		//! Count of agents bound to the dispatcher.
		std::atomic< std::size_t > & m_agents_bound;

		common_data_t(
			Work_Thread & work_thread,
			std::atomic< std::size_t > & agents_bound )
			:	m_work_thread( work_thread )
			,	m_agents_bound( agents_bound )
		{}
	};

inline void
track_activity(
	const mbox_t &,
	const common_data_t< work_thread::work_thread_no_activity_tracking_t > & )
	{}

inline void
track_activity(
	const mbox_t & mbox,
	const common_data_t< work_thread::work_thread_with_activity_tracking_t > & data )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				data.m_base_prefix,
				stats::suffixes::work_thread_activity(),
				data.m_work_thread.thread_id(),
				data.m_work_thread.take_activity_stats() );
	}

} /* namespace data_source_details */

/*!
 * \brief Data source for one-thread dispatcher.
 */
template< typename Work_Thread >
class data_source_t
	:	public stats::source_t
	,	protected data_source_details::common_data_t< Work_Thread >
	{
	public :
		using actual_work_thread_type_t = Work_Thread;

		//! SObjectizer Environment to work in.
		environment_t * m_env = { nullptr };

	public :
		data_source_t(
			actual_work_thread_type_t & work_thread,
			std::atomic< std::size_t > & agents_bound )
			:	data_source_details::common_data_t< Work_Thread >(
					work_thread, agents_bound )
			{}

		~data_source_t() override
			{
				if( m_env )
					stop();
			}

		void
		distribute( const mbox_t & mbox ) override
			{
				so_5::send< stats::messages::quantity< std::size_t > >(
						mbox,
						this->m_base_prefix,
						stats::suffixes::agent_count(),
						this->m_agents_bound.load( std::memory_order_acquire ) );

				so_5::send< stats::messages::quantity< std::size_t > >(
						mbox,
						this->m_work_thread_prefix,
						stats::suffixes::work_thread_queue_size(),
						this->m_work_thread.demands_count() );

				data_source_details::track_activity( mbox, *this );
			}

		void
		set_data_sources_name_base(
			const std::string & name_base,
			const void * disp_this_pointer )
			{
				using namespace so_5::disp::reuse;

				this->m_base_prefix = make_disp_prefix(
						"ot", // ot -- one_thread
						name_base,
						disp_this_pointer );

				this->m_work_thread_prefix = make_disp_working_thread_prefix(
						this->m_base_prefix,
						0 );
			}

		void
		start(
			environment_t & env )
			{
				env.stats_repository().add( *this );
				m_env = &env;
			}

		void
		stop()
			{
				m_env->stats_repository().remove( *this );
				m_env = nullptr;
			}
	};

//
// actual_disp_iface_t
//
/*!
 * \brief Interface of actual dispatcher.
 * \since
 * v.5.5.18
 */
class actual_disp_iface_t : public so_5::dispatcher_t
	{
	public :
		/*!
		 * \brief Get an event queue for new agent.
		 */
		virtual event_queue_t *
		get_agent_binding() = 0;

		/*!
		 * \brief Inform dispatcher about binding of yet another agent.
		 */
		virtual void
		agent_bound() = 0;

		/*!
		 * \brief Inform dispatcher abount unbinding of yet another agent.
		 */
		virtual void
		agent_unbound() = 0;
	};

//
// actual_dispatcher_t
//

/*!
	\brief A dispatcher with the single working thread and an event queue.
*/
template< typename Work_Thread >
class actual_dispatcher_t : public actual_disp_iface_t
	{
	public:
		actual_dispatcher_t( disp_params_t params )
			:	m_work_thread{ params.queue_params().lock_factory() }
			,	m_data_source( m_work_thread, m_agents_bound )
			{}

		//! \name Implementation of so_5::dispatcher methods.
		//! \{
		virtual void
		start( environment_t & env ) override
			{
				m_data_source.start( env );

				so_5::details::do_with_rollback_on_exception(
						[this] { m_work_thread.start(); },
						[this] { m_data_source.stop(); } );
			}

		virtual void
		shutdown() override
			{
				m_work_thread.shutdown();
			}

		virtual void
		wait() override
			{
				m_work_thread.wait();

				m_data_source.stop();
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override
			{
				m_data_source.set_data_sources_name_base( name_base, this );
			}
		//! \}

		//! \name Implementation of actial_disp_iface methods.
		//! \{
		virtual event_queue_t *
		get_agent_binding() override
			{
				return m_work_thread.get_agent_binding();
			}

		virtual void
		agent_bound() override
			{
				++m_agents_bound;
			}

		virtual void
		agent_unbound() override
			{
				--m_agents_bound;
			}

	private:
		//! Working thread for the dispatcher.
		Work_Thread m_work_thread;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Count of agents bound to this dispatcher.
		 */
		std::atomic< std::size_t > m_agents_bound = { 0 };

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Data source for run-time monitoring.
		 */
		data_source_t< Work_Thread > m_data_source;
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
		proxy_dispatcher_t( disp_params_t params )
			:	proxy_dispatcher_base_t( std::move(params) )
			{}

		virtual event_queue_t *
		get_agent_binding() override
			{
				return m_disp->get_agent_binding();
			}

		virtual void
		agent_bound() override
			{
				m_disp->agent_bound();
			}

		virtual void
		agent_unbound() override
			{
				m_disp->agent_unbound();
			}

	protected :
		virtual void
		do_actual_start( environment_t & env ) override
			{
				using dispatcher_no_activity_tracking_t =
						actual_dispatcher_t<
								work_thread::work_thread_no_activity_tracking_t >;

				using dispatcher_with_activity_tracking_t =
						actual_dispatcher_t<
								work_thread::work_thread_with_activity_tracking_t >;

				make_actual_dispatcher<
							dispatcher_no_activity_tracking_t,
							dispatcher_with_activity_tracking_t >(
						env,
						m_disp_params );
			}
	};

//
// binding_actions_mixin_t
//
/*!
 * \brief Implementation of binding actions to be reused
 * in various binder implementation.
 *
 * \since
 * v.5.5.4
 */
class binding_actions_mixin_t
	{
	protected :
		inline static disp_binding_activator_t
		do_bind(
			actual_disp_iface_t & disp,
			agent_ref_t agent )
			{
				auto result = [agent, &disp]() {
					agent->so_bind_to_dispatcher( *(disp.get_agent_binding()) );
				};

				// Dispatcher must know about yet another agent bound.
				disp.agent_bound();

				return result;
			}

		inline static void
		do_unbind(
			actual_disp_iface_t & disp,
			agent_ref_t /*agent*/)
			{
				// Dispatcher must know about yet another agent bound.
				disp.agent_unbound();
			}
	};

//
// disp_binder_t
//

//! Agent dispatcher binder.
class disp_binder_t
	:	public so_5::disp_binder_t
	,	private binding_actions_mixin_t
{
	public:
		explicit disp_binder_t(
			const std::string & disp_name )
			:	m_disp_name( disp_name )
		{}

		disp_binder_t(
			std::string && disp_name )
			:	m_disp_name( std::move(disp_name) )
		{}

		virtual disp_binding_activator_t
		bind_agent(
			environment_t & env,
			agent_ref_t agent_ref ) override
		{
			return make_agent_binding( 
				env.query_named_dispatcher( m_disp_name ).get(),
				std::move( agent_ref ) );
		}

		virtual void
		unbind_agent(
			environment_t & env,
			agent_ref_t agent_ref ) override
		{
			return unbind_agent_from_disp( 
				env.query_named_dispatcher( m_disp_name ).get(),
				std::move( agent_ref ) );
		}

	private:
		//! Name of the dispatcher to be bound to.
		/*!
		 * Empty name means usage of default dispatcher.
		 */
		const std::string m_disp_name;

		/*!
		 * \brief Make binding to the dispatcher specified.
		 *
		 * \since
		 * v.5.4.0
		 */
		disp_binding_activator_t
		make_agent_binding(
			so_5::dispatcher_t * disp,
			agent_ref_t agent )
		{
			using namespace so_5::disp::reuse;

			return do_with_dispatcher_of_type< actual_disp_iface_t >(
					disp,
					m_disp_name,
					[agent]( actual_disp_iface_t & d )
					{
						return do_bind( d, std::move(agent) );
					} );
		}

		/*!
		 * \brief Unbind agent from the dispatcher specified.
		 *
		 * \since
		 * v.5.5.4
		 */
		void
		unbind_agent_from_disp(
			so_5::dispatcher_t * disp,
			agent_ref_t agent )
		{
			using namespace so_5::disp::reuse;

			do_with_dispatcher_of_type< actual_disp_iface_t >(
					disp,
					m_disp_name,
					[agent]( actual_disp_iface_t & d ) {
						do_unbind( d, std::move(agent) );
					} );
		}
};

//
// disp_binder_for_specific_dispatcher_t
//

//! Agent dispatcher binder.
/*!
 * \since
 * v.5.5.19
 */
class disp_binder_for_specific_dispatcher_t
	:	public so_5::disp_binder_t
	,	private binding_actions_mixin_t
{
	public:
		explicit disp_binder_for_specific_dispatcher_t(
			actual_disp_iface_t & dispatcher )
			:	m_dispatcher( dispatcher )
		{}

		virtual disp_binding_activator_t
		bind_agent(
			environment_t & /*env*/,
			agent_ref_t agent ) override
		{
			return do_bind( m_dispatcher, std::move(agent) );
		}

		virtual void
		unbind_agent(
			environment_t & /*env*/,
			agent_ref_t agent ) override
		{
			do_unbind( m_dispatcher, std::move(agent) );
		}

	private:
		//! Dispatcher to be bound to.
		actual_disp_iface_t & m_dispatcher;
};

//
// private_dispatcher_binder_t
//

/*!
 * \brief A binder for the private %one_thread dispatcher.
 *
 * \since
 * v.5.5.4, v.5.5.18
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
 * v.5.5.4
 */
class real_private_dispatcher_t : public private_dispatcher_t
	{
	public :
		/*!
		 * Constructor creates a dispatcher instance and launches it.
		 */
		real_private_dispatcher_t(
			//! SObjectizer Environment to work in.
			environment_t & env,
			//! Value for creating names of data sources for
			//! run-time monitoring.
			const std::string & data_sources_name_base,
			//! Parameters for the dispatcher,
			disp_params_t params )
			:	m_disp( new proxy_dispatcher_t{ std::move(params) } )
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
				return disp_binder_unique_ptr_t(
						new private_dispatcher_binder_t(
								private_dispatcher_handle_t( this ),
								*m_disp ) );
			}

	private :
		std::unique_ptr< proxy_dispatcher_t > m_disp;
	};

} /* namespace impl */

//
// create_disp
//
SO_5_FUNC dispatcher_unique_ptr_t
create_disp( disp_params_t params )
{
	return dispatcher_unique_ptr_t(
			new impl::proxy_dispatcher_t{ std::move(params) } );
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
			new impl::real_private_dispatcher_t(
					env,
					data_sources_name_base,
					std::move(params) ) );
}

//
// create_disp_binder
//
SO_5_FUNC disp_binder_unique_ptr_t
create_disp_binder( nonempty_name_t disp_name )
{
	return disp_binder_unique_ptr_t(
		new impl::disp_binder_t( disp_name.giveout_value() ) );
}

namespace internals {

disp_binder_unique_ptr_t
create_binder_for_specific_dispatcher(
	so_5::dispatcher_t & dispatcher )
{
	using namespace so_5::disp::one_thread::impl;

	actual_disp_iface_t & actual_dispatcher =
		dynamic_cast< actual_disp_iface_t & >(dispatcher);

	return stdcpp::make_unique< disp_binder_for_specific_dispatcher_t >(
			std::ref(actual_dispatcher) );
}

} /* namespace internals */

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */

