/*
	SObjectizer 5
*/

#include <so_5/disp/active_obj/h/pub.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/send_functions.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/h/proxy_dispatcher_template.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/h/stdcpp.hpp>

#include <map>
#include <mutex>
#include <algorithm>

namespace so_5
{

namespace disp
{

namespace active_obj
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::stats;

namespace
{

/*!
 * \brief Just a helper function for consequetive call to shutdown and wait.
 * \since
 * v.5.5.4
 */
template< class T >
void
shutdown_and_wait( T & w )
	{
		w.shutdown();
		w.wait();
	}

template< class T >
void
call_shutdown( T & agent_thread )
{
	agent_thread.second->shutdown();
}

template< class T >
void
call_wait( T & agent_thread )
{
	agent_thread.second->wait();
}

template< class WORK_THREAD >
void
send_demands_count_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	WORK_THREAD & wt )
	{
		so_5::send< stats::messages::quantity< std::size_t > >(
				mbox,
				prefix,
				stats::suffixes::work_thread_queue_size(),
				wt.demands_count() );
	}

void
send_thread_activity_stats(
	const so_5::mbox_t &,
	const stats::prefix_t &,
	work_thread::work_thread_no_activity_tracking_t & )
	{
		/* Nothing to do */
	}

void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	work_thread::work_thread_with_activity_tracking_t & wt )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				prefix,
				stats::suffixes::work_thread_activity(),
				wt.thread_id(),
				wt.take_activity_stats() );
	}

} /* anonymous */

//
// actual_disp_iface_t
//
/*!
 * \brief An actual interface of active object dispatcher.
 *
 * \since
 * v.5.5.18
 */
class actual_disp_iface_t : public so_5::dispatcher_t
	{
	public :
		//! Creates a new thread for the agent specified.
		virtual event_queue_t *
		create_thread_for_agent( const agent_t & agent ) = 0;

		//! Destroys the thread for the agent specified.
		virtual void
		destroy_thread_for_agent( const agent_t & agent ) = 0;
	};

//
// dispatcher_template_t
//

/*!
 * \brief Implementation of active object dispatcher in form of template class.
 */
template< typename WORK_THREAD >
class dispatcher_template_t : public actual_disp_iface_t
	{
	public:
		dispatcher_template_t( disp_params_t params )
			:	m_params{ std::move(params) }
			,	m_data_source( self() )
			{}

		virtual void
		start( environment_t & env ) override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				m_data_source.start( outliving_mutable(env.stats_repository()) );

				so_5::details::do_with_rollback_on_exception(
					[this] { m_shutdown_started = false; },
					[this] { m_data_source.stop(); } );
			}

		virtual void
		shutdown() override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				// During the shutdown new threads will not be created.
				m_shutdown_started = true;

				std::for_each(
					m_agent_threads.begin(),
					m_agent_threads.end(),
					call_shutdown< typename agent_thread_map_t::value_type > );
			}

		virtual void
		wait() override
			{
				std::for_each(
					m_agent_threads.begin(),
					m_agent_threads.end(),
					call_wait< typename agent_thread_map_t::value_type > );

				m_data_source.stop();
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override
			{
				m_data_source.set_data_sources_name_base( name_base );
			}

		virtual event_queue_t *
		create_thread_for_agent( const agent_t & agent ) override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				if( m_shutdown_started )
					throw so_5::exception_t(
						"shutdown was initiated",
						rc_disp_create_failed );

				if( m_agent_threads.end() != m_agent_threads.find( &agent ) )
					throw so_5::exception_t(
						"thread for the agent is already exists",
						rc_disp_create_failed );

				auto lock_factory = m_params.queue_params().lock_factory();
				auto thread = std::make_shared< WORK_THREAD >(
						std::move(lock_factory) );

				thread->start();
				so_5::details::do_with_rollback_on_exception(
						[&] { m_agent_threads[ &agent ] = thread; },
						[&thread] { shutdown_and_wait( *thread ); } );

				return thread->get_agent_binding();
			}

		void
		destroy_thread_for_agent( const agent_t & agent ) override
			{
				auto thread = search_and_remove_agent_from_map( agent );
				if( thread )
					shutdown_and_wait( *thread );
			}

	private:
		friend class disp_data_source_t;

		//! An alias for shared pointer to work thread object.
		using work_thread_shptr_t = std::shared_ptr< WORK_THREAD >;

		//! Typedef for mapping from agents to their working threads.
		using agent_thread_map_t =
				std::map< const agent_t *, work_thread_shptr_t >;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 * \since
		 * v.5.5.4
		 */
		class disp_data_source_t : public stats::manually_registered_source_t
			{
				//! Dispatcher to work with.
				dispatcher_template_t & m_dispatcher;

				//! Basic prefix for data source names.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t( dispatcher_template_t & disp )
					:	m_dispatcher( disp )
					{}

				virtual void
				distribute( const mbox_t & mbox )
					{
						std::lock_guard< std::mutex > lock{ m_dispatcher.m_lock };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								m_dispatcher.m_agent_threads.size() );

						for( const auto & p : m_dispatcher.m_agent_threads )
							distribute_value_for_work_thread(
									mbox,
									p.first,
									*p.second );
					}

				void
				set_data_sources_name_base(
					const std::string & name_base )
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								"ao", // ao -- active_objects
								name_base,
								&m_dispatcher );
					}

			private:
				void
				distribute_value_for_work_thread(
					const mbox_t & mbox,
					const agent_t * agent,
					WORK_THREAD & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-"
								<< so_5::disp::reuse::ios_helpers::pointer{ agent };

						const stats::prefix_t wt_prefix{ ss.str() };

						send_demands_count_stats( mbox, wt_prefix, wt );
						send_thread_activity_stats( mbox, wt_prefix, wt );
					}
			};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

		/*!
		 * \brief Parameters for the dispatcher.
		 *
		 * \since
		 * v.5.5.10
		 */
		disp_params_t m_params;

		//! A map from agents to single thread dispatchers.
		agent_thread_map_t m_agent_threads;

		//! Shutdown flag.
		bool m_shutdown_started = { false };

		//! This object lock.
		std::mutex m_lock;

		/*!
		 * \brief Data source for run-time monitoring.
		 *
		 * \since
		 * v.5.5.4
		 */
		disp_data_source_t m_data_source;

		/*!
		 * \brief Helper function for searching and erasing agent's
		 * thread from map of active threads.
		 *
		 * \note Does all actions on locked object.
		 *
		 * \since
		 * v.5.5.4
		 */
		work_thread_shptr_t
		search_and_remove_agent_from_map(
			const agent_t & agent )
			{
				work_thread_shptr_t result;

				std::lock_guard< std::mutex > lock( m_lock );

				if( !m_shutdown_started )
				{
					auto it = m_agent_threads.find( &agent );

					if( m_agent_threads.end() != it )
					{
						result = it->second;
						m_agent_threads.erase( it );
					}
				}

				return result;
			}

		/*!
		 * \brief Just a helper method for getting reference to itself.
		 *
		 * \since
		 * v.5.5.4
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
		proxy_dispatcher_t( disp_params_t params )
			:	proxy_dispatcher_base_t( std::move(params) )
			{}

		virtual event_queue_t *
		create_thread_for_agent( const agent_t & agent ) override
			{
				return m_disp->create_thread_for_agent( agent );
			}

		virtual void
		destroy_thread_for_agent( const agent_t & agent ) override
			{
				return m_disp->destroy_thread_for_agent( agent );
			}

	protected:
		virtual void
		do_actual_start( environment_t & env ) override
			{
				using namespace work_thread;

				using dispatcher_no_activity_tracking_t =
						dispatcher_template_t< work_thread_no_activity_tracking_t >;

				using dispatcher_with_activity_tracking_t =
						dispatcher_template_t<
								work_thread_with_activity_tracking_t >;

				make_actual_dispatcher<
							dispatcher_no_activity_tracking_t,
							dispatcher_with_activity_tracking_t >(
						env,
						m_disp_params );
			}
	};

//
// binding_actions_t
//
/*!
 * \brief A mixin with implementation of main binding/unbinding actions.
 *
 * \since
 * v.5.5.4
 */
class binding_actions_t
	{
	protected :
		disp_binding_activator_t
		do_bind(
			actual_disp_iface_t & disp,
			agent_ref_t agent )
			{
				auto ctx = disp.create_thread_for_agent( *agent );

				return so_5::details::do_with_rollback_on_exception(
						[&] {
							return disp_binding_activator_t{
								[agent, ctx]() {
									agent->so_bind_to_dispatcher( *ctx );
								} };
						},
						[&] {
							// Dispatcher for the agent should be removed.
							disp.destroy_thread_for_agent( *agent );
						} );
			}

		void
		do_unbind(
			actual_disp_iface_t & disp,
			agent_ref_t agent )
			{
				disp.destroy_thread_for_agent( *agent );
			}
	};

//
// disp_binder_t
//

//! Agent dispatcher binder.
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		proxy_dispatcher_t, binding_actions_t >;

//
// private_dispatcher_binder_t
//

/*!
 * \since
 * v.5.5.4
 *
 * \brief A binder for the private %active_obj dispatcher.
 */
using private_dispatcher_binder_t =
	so_5::disp::reuse::binder_for_private_disp_template_t<
		private_dispatcher_handle_t,
		proxy_dispatcher_t,
		binding_actions_t >;

//
// real_private_dispatcher_t
//
/*!
 * \since
 * v.5.5.4
 *
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
			:	m_disp( so_5::stdcpp::make_unique< proxy_dispatcher_t >(
					std::move( params ) ) )
			{
				m_disp->set_data_sources_name_base( data_sources_name_base );
				m_disp->start( env );
			}

		/*!
		 * Destructors shuts an instance down and waits for it.
		 */
		~real_private_dispatcher_t()
			{
				shutdown_and_wait( *m_disp );
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
// private_dispatcher_t
//
private_dispatcher_t::~private_dispatcher_t()
	{}

//
// create_disp
//
SO_5_FUNC dispatcher_unique_ptr_t
create_disp(
	disp_params_t params )
	{
		return so_5::stdcpp::make_unique< impl::proxy_dispatcher_t >(
				std::move(params) );
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
create_disp_binder( const std::string & disp_name )
	{
		return so_5::stdcpp::make_unique< impl::disp_binder_t >( disp_name );
	}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */

