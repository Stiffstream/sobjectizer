/*
	SObjectizer 5.
*/

#include <so_5/disp/active_group/h/pub.hpp>

#include <so_5/disp.hpp>
#include <so_5/send_functions.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>
#include <so_5/disp/reuse/h/proxy_dispatcher_template.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

#include <so_5/stats/repository.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/stdcpp.hpp>

#include <map>
#include <mutex>
#include <algorithm>

namespace so_5
{

namespace disp
{

namespace active_group
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::stats;

namespace
{

/*!
 * \brief Just a helper function for consequetive call to shutdown and wait.
 *
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
call_shutdown( T & v )
	{
		v.second.m_thread->shutdown();
	}

template< class T >
void
call_wait( T & v )
	{
		v.second.m_thread->wait();
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
 * \brief An actual interface of active group dispatcher.
 *
 * \since
 * v.5.5.18
 */
class actual_disp_iface_t : public so_5::dispatcher_t
	{
	public :
		/*!
		 * \brief Get the event_queue for the specified active group.
		 *
		 * If name \a group_name is unknown then a new work
		 * thread is started. This thread is marked as it has one
		 * working agent on it.
		 *
		 * If there already is a thread for \a group_name then the
		 * counter of working agents for it is incremented.
		 */
		virtual so_5::event_queue_t *
		query_thread_for_group( const std::string & group_name ) = 0;

		/*!
		 * \brief Release the thread for the specified active group.
		 *
		 * Method decrements the working agent count for the thread of
		 * \a group_name. If there no more working agents left then
		 * the event_queue and working thread for that group will be
		 * destroyed.
		 */
		virtual void
		release_thread_for_group( const std::string & group_name ) = 0;
	};

//
// dispatcher_template_t
//

/*!
 * \brief Implementation of active object dispatcher in form of template class.
 */
template< typename Work_Thread >
class dispatcher_template_t : public actual_disp_iface_t
	{
	public:
		dispatcher_template_t( disp_params_t params )
			:	m_params{ std::move(params) }
			,	m_data_source( self() )
			{}

		virtual void
		start( so_5::environment_t & env ) override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				m_data_source.start( outliving_mutable(env.stats_repository()) );

				so_5::details::do_with_rollback_on_exception(
					[this] {
						// do_with_rollback_on_exception is used in this simple case
						// just for simplification of extension of that block
						// in the future.
						m_shutdown_started = false;
					},
					[this] { m_data_source.stop(); } );
			}

		virtual void
		shutdown() override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				// Starting shutdown process.
				// New groups will not be created. But old groups remain.
				m_shutdown_started = true;

				std::for_each(
					m_groups.begin(),
					m_groups.end(),
					call_shutdown< typename active_group_map_t::value_type > );
			}

		virtual void
		wait() override
			{
				std::for_each(
					m_groups.begin(),
					m_groups.end(),
					call_wait< typename active_group_map_t::value_type > );

				m_data_source.stop();
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override
			{
				m_data_source.set_data_sources_name_base( name_base );
			}

		virtual so_5::event_queue_t *
		query_thread_for_group( const std::string & group_name ) override
			{
				std::lock_guard< std::mutex > lock( m_lock );

				if( m_shutdown_started )
					throw so_5::exception_t(
						"shutdown was initiated",
						rc_disp_create_failed );

				auto it = m_groups.find( group_name );

				// If there is a thread for an active group it should be returned.
				if( m_groups.end() != it )
					{
						++(it->second.m_user_agent);
						return it->second.m_thread->get_agent_binding();
					}

				// New thread should be created.
				auto thread = std::make_shared< Work_Thread >(
						m_params.queue_params().lock_factory() );

				thread->start();

				so_5::details::do_with_rollback_on_exception(
						[&] {
							m_groups.emplace(
									group_name,
									thread_with_refcounter_t( thread, 1u ) );
						},
						[&thread] { shutdown_and_wait( *thread ); } );

				return thread->get_agent_binding();
			}

		void
		release_thread_for_group( const std::string & group_name ) override
			{
				auto thread = search_and_try_remove_group_from_map( group_name );
				if( thread )
					shutdown_and_wait( *thread );
			}

	private:
		friend class disp_data_source_t;

		//! An alias for shared pointer to work thread.
		using work_thread_shptr_t = std::shared_ptr< Work_Thread >;

		//! Auxiliary class for the working agent counting.
		struct thread_with_refcounter_t
			{
				thread_with_refcounter_t(
					work_thread_shptr_t thread,
					std::size_t user_agent )
					:	m_thread( std::move( thread ) )
					,	m_user_agent( user_agent)
					{}

				work_thread_shptr_t m_thread;
				std::size_t m_user_agent;
			};

		//! Typedef for mapping from group names to a single thread
		//! dispatcher.
		typedef std::map< std::string, thread_with_refcounter_t >
			active_group_map_t;

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since
		 * v.5.5.4
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
				distribute( const so_5::mbox_t & mbox ) override
					{
						std::lock_guard< std::mutex > lock{ m_dispatcher.m_lock };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::disp_active_group_count(),
								m_dispatcher.m_groups.size() );

						std::size_t agent_count = 0;
						for( const auto & p : m_dispatcher.m_groups )
							{
								distribute_value_for_work_thread(
										mbox,
										p.first,
										p.second );

								agent_count += p.second.m_user_agent;
							}

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								agent_count );
					}

				void
				set_data_sources_name_base(
					const std::string & name_base )
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								"ag", // ao -- active_groups
								name_base,
								&m_dispatcher );
					}

			private:
				void
				distribute_value_for_work_thread(
					const so_5::mbox_t & mbox,
					const std::string & group_name,
					const thread_with_refcounter_t & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-" << group_name;

						const stats::prefix_t prefix{ ss.str() };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::agent_count(),
								wt.m_user_agent );

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								prefix,
								stats::suffixes::work_thread_queue_size(),
								wt.m_thread->demands_count() );

						send_thread_activity_stats(
								mbox,
								prefix,
								*(wt.m_thread) );
					}
			};

		//! Parameters for the dispatcher.
		const disp_params_t m_params;

		//! A map of dispatchers for active groups.
		active_group_map_t m_groups;

		//! Shutdown of the indication flag.
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
		 * \since
		 * v.5.5.4
		 *
		 * \note Does all actions on locked object.
		 *
		 * \return nullptr if thread for the group is not found
		 * or there are still some agents on it.
		 */
		work_thread_shptr_t
		search_and_try_remove_group_from_map(
			const std::string & group_name )
			{
				work_thread_shptr_t result;

				std::lock_guard< std::mutex > lock( m_lock );

				if( !m_shutdown_started )
					{
						auto it = m_groups.find( group_name );

						if( m_groups.end() != it && 0 == --(it->second.m_user_agent) )
							{
								result = it->second.m_thread;
								m_groups.erase( it );
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

		virtual so_5::event_queue_t *
		query_thread_for_group( const std::string & group_name ) override
			{
				return m_disp->query_thread_for_group( group_name );
			}

		virtual void
		release_thread_for_group( const std::string & group_name ) override
			{
				return m_disp->release_thread_for_group( group_name );
			}

	protected :
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
		binding_actions_t( std::string group_name )
			:	m_group_name( std::move( group_name ) )
			{}

		so_5::disp_binding_activator_t
		do_bind(
			actual_disp_iface_t & disp,
			so_5::agent_ref_t agent )
			{
				auto ctx = disp.query_thread_for_group( m_group_name );

				return so_5::details::do_with_rollback_on_exception(
						[&] {
							return so_5::disp_binding_activator_t {
								[agent, ctx]() {
									agent->so_bind_to_dispatcher( *ctx );
								} };
						},
						[&] {
							// Dispatcher for the agent should be removed.
							disp.release_thread_for_group( m_group_name );
						} );
			}

		void
		do_unbind(
			actual_disp_iface_t & disp,
			so_5::agent_ref_t /*agent*/ )
			{
				disp.release_thread_for_group( m_group_name );
			}

	private :
		const std::string m_group_name;
	};

//
// disp_binder_t
//

//! Agent dispatcher binder interface.
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		proxy_dispatcher_t, binding_actions_t >;

//
// private_dispatcher_binder_t
//
/*!
 * \brief A binder for the private %active_group dispatcher.
 *
 * \since
 * v.5.5.4
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
 * \brief A real implementation of private_dispatcher interface.
 *
 * \since
 * v.5.5.4
 */
class real_private_dispatcher_t : public private_dispatcher_t
	{
	public :
		/*!
		 * Constructor creates a dispatcher instance and launces it.
		 */
		real_private_dispatcher_t(
			//! SObjectizer Environment to work in.
			so_5::environment_t & env,
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
		~real_private_dispatcher_t() override
			{
				shutdown_and_wait( *m_disp );
			}

		virtual so_5::disp_binder_unique_ptr_t
		binder( const std::string & group_name ) override
			{
				return so_5::stdcpp::make_unique< private_dispatcher_binder_t >(
						private_dispatcher_handle_t( this ),
						*m_disp,
						group_name );
			}

	private :
		std::unique_ptr< proxy_dispatcher_t > m_disp;
	};

} /* namespace impl */

//
// create_disp
//
SO_5_FUNC so_5::dispatcher_unique_ptr_t
create_disp( disp_params_t params )
	{
		return so_5::stdcpp::make_unique< impl::proxy_dispatcher_t >(
				std::move(params) );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	so_5::environment_t & env,
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
SO_5_FUNC so_5::disp_binder_unique_ptr_t
create_disp_binder(
	const std::string & disp_name,
	const std::string & group_name )
	{
		return so_5::stdcpp::make_unique< impl::disp_binder_t >(
				disp_name, group_name );
	}

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */

