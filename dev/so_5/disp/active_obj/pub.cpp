/*
	SObjectizer 5
*/

#include <so_5/disp/active_obj/h/pub.hpp>

#include <map>
#include <mutex>
#include <algorithm>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/send_functions.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

#include <so_5/rt/stats/h/repository.hpp>
#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

namespace so_5
{

namespace disp
{

namespace active_obj
{

namespace impl
{

namespace work_thread = so_5::disp::reuse::work_thread;
namespace stats = so_5::rt::stats;

namespace
{

/*!
 * \since v.5.5.4
 * \brief Just a helper function for consequetive call to shutdown and wait.
 */
template< class T >
void
shutdown_and_wait( T & w )
	{
		w.shutdown();
		w.wait();
	}

} /* anonymous */

//
// dispatcher_t
//

/*!
	\brief Active objects dispatcher.
*/
class dispatcher_t : public so_5::rt::dispatcher_t
{
	public:
		dispatcher_t( params_t params );

		//! \name Implemetation of so_5::rt::dispatcher methods.
		//! \{
		virtual void
		start( so_5::rt::environment_t & env ) override;

		virtual void
		shutdown() override;

		virtual void
		wait() override;

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override;
		//! \}

		//! Creates a new thread for the agent specified.
		so_5::rt::event_queue_t *
		create_thread_for_agent( const so_5::rt::agent_t & agent );

		//! Destroys the thread for the agent specified.
		void
		destroy_thread_for_agent( const so_5::rt::agent_t & agent );

	private:
		friend class disp_data_source_t;

		//! Typedef for mapping from agents to their working threads.
		typedef std::map<
				const so_5::rt::agent_t *,
				work_thread::work_thread_shptr_t >
			agent_thread_map_t;

		/*!
		 * \since v.5.5.4
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 */
		class disp_data_source_t : public stats::manually_registered_source_t
			{
				//! Dispatcher to work with.
				dispatcher_t & m_dispatcher;

				//! Basic prefix for data source names.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t( dispatcher_t & disp )
					:	m_dispatcher( disp )
					{}

				virtual void
				distribute( const so_5::rt::mbox_t & mbox )
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
					const so_5::rt::mbox_t & mbox,
					const so_5::rt::agent_t * agent,
					work_thread::work_thread_t & wt )
					{
						std::ostringstream ss;
						ss << m_base_prefix.c_str() << "/wt-"
								<< so_5::disp::reuse::ios_helpers::pointer{ agent };

						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								stats::prefix_t{ ss.str() },
								stats::suffixes::work_thread_queue_size(),
								wt.demands_count() );
					}
			};

		/*!
		 * \since v.5.5.10
		 * \brief Parameters for the dispatcher.
		 */
		params_t m_params;

		//! A map from agents to single thread dispatchers.
		agent_thread_map_t m_agent_threads;

		//! Shutdown flag.
		bool m_shutdown_started = { false };

		//! This object lock.
		std::mutex m_lock;

		/*!
		 * \since v.5.5.4
		 * \brief Data source for run-time monitoring.
		 */
		disp_data_source_t m_data_source;

		/*!
		 * \since v.5.5.4
		 * \brief Helper function for searching and erasing agent's
		 * thread from map of active threads.
		 *
		 * \note Does all actions on locked object.
		 */
		so_5::disp::reuse::work_thread::work_thread_shptr_t
		search_and_remove_agent_from_map(
			const so_5::rt::agent_t & agent );

		/*!
		 * \since v.5.5.4
		 * \brief Just a helper method for getting reference to itself.
		 */
		dispatcher_t &
		self()
			{
				return *this;
			}
};

dispatcher_t::dispatcher_t( params_t params )
	:	m_params{ std::move(params) }
	,	m_data_source( self() )
{
}

void
dispatcher_t::start( so_5::rt::environment_t & env )
{
	std::lock_guard< std::mutex > lock( m_lock );

	m_data_source.start( env.stats_repository() );

	so_5::details::do_with_rollback_on_exception(
		[this] { m_shutdown_started = false; },
		[this] { m_data_source.stop(); } );
}

template< class T >
void
call_shutdown( T & agent_thread )
{
	agent_thread.second->shutdown();
}

void
dispatcher_t::shutdown()
{
	std::lock_guard< std::mutex > lock( m_lock );

	// During the shutdown new threads will not be created.
	m_shutdown_started = true;

	std::for_each(
		m_agent_threads.begin(),
		m_agent_threads.end(),
		call_shutdown< agent_thread_map_t::value_type > );
}

template< class T >
void
call_wait( T & agent_thread )
{
	agent_thread.second->wait();
}

void
dispatcher_t::wait()
{
	std::for_each(
		m_agent_threads.begin(),
		m_agent_threads.end(),
		call_wait< agent_thread_map_t::value_type > );

	m_data_source.stop();
}

void
dispatcher_t::set_data_sources_name_base(
	const std::string & name_base )
	{
		m_data_source.set_data_sources_name_base( name_base );
	}

so_5::rt::event_queue_t *
dispatcher_t::create_thread_for_agent( const so_5::rt::agent_t & agent )
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

	using namespace so_5::disp::reuse::work_thread;

	auto lock_factory = m_params.queue_params().lock_factory();
	work_thread_shptr_t thread( new work_thread_t{ std::move(lock_factory) } );

	thread->start();
	so_5::details::do_with_rollback_on_exception(
			[&] { m_agent_threads[ &agent ] = thread; },
			[&thread] { shutdown_and_wait( *thread ); } );

	return thread->get_agent_binding();
}

void
dispatcher_t::destroy_thread_for_agent( const so_5::rt::agent_t & agent )
{
	auto thread = search_and_remove_agent_from_map( agent );
	if( thread )
		shutdown_and_wait( *thread );
}

so_5::disp::reuse::work_thread::work_thread_shptr_t
dispatcher_t::search_and_remove_agent_from_map(
	const so_5::rt::agent_t & agent )
{
	so_5::disp::reuse::work_thread::work_thread_shptr_t result;

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

//
// binding_actions_t
//
/*!
 * \since v.5.5.4
 * \brief A mixin with implementation of main binding/unbinding actions.
 */
class binding_actions_t
	{
	protected :
		so_5::rt::disp_binding_activator_t
		do_bind(
			dispatcher_t & disp,
			so_5::rt::agent_ref_t agent )
			{
				auto ctx = disp.create_thread_for_agent( *agent );

				return so_5::details::do_with_rollback_on_exception(
						[&] {
							return so_5::rt::disp_binding_activator_t{
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
			dispatcher_t & disp,
			so_5::rt::agent_ref_t agent )
			{
				disp.destroy_thread_for_agent( *agent );
			}
	};

//
// disp_binder_t
//

//! Agent dispatcher binder.
using disp_binder_t = so_5::disp::reuse::binder_for_public_disp_template_t<
		dispatcher_t, binding_actions_t >;

//
// private_dispatcher_binder_t
//

/*!
 * \since v.5.5.4
 * \brief A binder for the private %active_obj dispatcher.
 */
using private_dispatcher_binder_t =
	so_5::disp::reuse::binder_for_private_disp_template_t<
		private_dispatcher_handle_t,
		dispatcher_t,
		binding_actions_t >;

//
// real_private_dispatcher_t
//
/*!
 * \since v.5.5.4
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
			params_t params )
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
				shutdown_and_wait( *m_disp );
			}

		virtual so_5::rt::disp_binder_unique_ptr_t
		binder() override
			{
				return so_5::rt::disp_binder_unique_ptr_t(
						new private_dispatcher_binder_t(
								private_dispatcher_handle_t( this ),
								*m_disp ) );
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
create_disp(
	params_t params )
	{
		return so_5::rt::dispatcher_unique_ptr_t(
				new impl::dispatcher_t{ std::move(params) } );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	so_5::rt::environment_t & env,
	const std::string & data_sources_name_base,
	params_t params )
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
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder( const std::string & disp_name )
	{
		return so_5::rt::disp_binder_unique_ptr_t( 
			new impl::disp_binder_t( disp_name ) );
	}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */

