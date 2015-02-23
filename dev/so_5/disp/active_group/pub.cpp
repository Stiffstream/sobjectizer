/*
	SObjectizer 5.
*/

#include <so_5/disp/active_group/h/pub.hpp>

#include <map>
#include <mutex>
#include <algorithm>

#include <so_5/rt/h/disp.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5
{

namespace disp
{

namespace active_group
{

namespace impl
{

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
	\brief Active group dispatcher.
*/
class dispatcher_t : public so_5::rt::dispatcher_t
{
	public:
		dispatcher_t();

		//! \name Implementation of so_5::rt::dispatcher methods.
		//! \{

		//! Launch the dispatcher.
		virtual void
		start();

		//! Send a signal about shutdown to the dispatcher.
		virtual void
		shutdown();

		//! Wait for the full stop of the dispatcher.
		virtual void
		wait();
		//! \}

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
		so_5::rt::event_queue_t *
		query_thread_for_group( const std::string & group_name );

		/*!
		 * \brief Release the thread for the specified active group.
		 *
		 * Method decrements the working agent count for the thread of
		 * \a group_name. If there no more working agents left then
		 * the event_queue and working thread for that group will be
		 * destroyed.
		 */
		void
		release_thread_for_group( const std::string & group_name );

	private:
		//! Auxiliary class for the working agent counting.
		struct thread_with_refcounter_t
		{
			thread_with_refcounter_t(
				so_5::disp::reuse::work_thread::work_thread_shptr_t thread,
				unsigned int user_agent )
				:
					m_thread( std::move( thread ) ),
					m_user_agent( user_agent)
			{}

			so_5::disp::reuse::work_thread::work_thread_shptr_t m_thread;
			unsigned int m_user_agent;
		};

		//! Typedef for mapping from group names to a single thread
		//! dispatcher.
		typedef std::map<
				std::string,
				thread_with_refcounter_t >
			active_group_map_t;

		//! A map of dispatchers for active groups.
		active_group_map_t m_groups;

		//! Shutdown of the indication flag.
		bool m_shutdown_started;

		//! This object lock.
		std::mutex m_lock;

		/*!
		 * \since v.5.5.4
		 * \brief Helper function for searching and erasing agent's
		 * thread from map of active threads.
		 *
		 * \note Does all actions on locked object.
		 *
		 * \return nullptr if thread for the group is not found
		 * or there are still some agents on it.
		 */
		so_5::disp::reuse::work_thread::work_thread_shptr_t
		search_and_try_remove_group_from_map(
			const std::string & group_name );
};

dispatcher_t::dispatcher_t()
{
}

void
dispatcher_t::start()
{
	std::lock_guard< std::mutex > lock( m_lock );
	m_shutdown_started = false;
}

template< class T >
void
call_shutdown( T & v )
{
	v.second.m_thread->shutdown();
}

void
dispatcher_t::shutdown()
{
	std::lock_guard< std::mutex > lock( m_lock );

	// Starting shutdown process.
	// New groups will not be created. But old groups remain.
	m_shutdown_started = true;

	std::for_each(
		m_groups.begin(),
		m_groups.end(),
		call_shutdown< active_group_map_t::value_type > );
}

template< class T >
void
call_wait( T & v )
{
	v.second.m_thread->wait();
}

void
dispatcher_t::wait()
{
	std::for_each(
		m_groups.begin(),
		m_groups.end(),
		call_wait< active_group_map_t::value_type > );
}

so_5::rt::event_queue_t *
dispatcher_t::query_thread_for_group( const std::string & group_name )
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
	using namespace so_5::disp::reuse::work_thread;

	work_thread_shptr_t thread( new work_thread_t() );
	thread->start();

	try
	{
		m_groups.insert(
				active_group_map_t::value_type(
						group_name,
						thread_with_refcounter_t( thread, 1 ) ) );
	}
	catch( ... )
	{
		shutdown_and_wait( *thread );
		throw;
	}

	return thread->get_agent_binding();
}

void
dispatcher_t::release_thread_for_group( const std::string & group_name )
{
	auto thread = search_and_try_remove_group_from_map( group_name );
	if( thread )
		shutdown_and_wait( *thread );
}

so_5::disp::reuse::work_thread::work_thread_shptr_t
dispatcher_t::search_and_try_remove_group_from_map(
	const std::string & group_name )
{
	so_5::disp::reuse::work_thread::work_thread_shptr_t result;

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
		binding_actions_t( std::string group_name )
			:	m_group_name( std::move( group_name ) )
			{}

		so_5::rt::disp_binding_activator_t
		do_bind(
			dispatcher_t & disp,
			so_5::rt::agent_ref_t agent )
			{
				auto ctx = disp.query_thread_for_group( m_group_name );

				try
				{
					so_5::rt::disp_binding_activator_t activator =
						[agent, ctx]() {
							agent->so_bind_to_dispatcher( *ctx );
						};

					return activator;
				}
				catch( ... )
				{
					// Dispatcher for the agent should be removed.
					disp.release_thread_for_group( m_group_name );
					throw;
				}
			}

		void
		do_unbind(
			dispatcher_t & disp )
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
class disp_binder_t
	:	public so_5::rt::disp_binder_t
	,	protected binding_actions_t
	{
	public:
		disp_binder_t(
			const std::string & disp_name,
			const std::string & group_name )
			:	binding_actions_t( group_name )
			,	m_disp_name( disp_name )
			{}

		//! Bind agent to a dispatcher.
		virtual so_5::rt::disp_binding_activator_t
		bind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent ) override
			{
				using so_5::rt::disp_binding_activator_t;
				using namespace so_5::disp::reuse;

				return do_with_dispatcher< disp_binding_activator_t, dispatcher_t >(
					env,
					m_disp_name,
					[this, agent]( dispatcher_t & disp )
					{
						return do_bind( disp, std::move( agent ) );
					} );
			}

		//! Unbind agent from the dispatcher.
		virtual void
		unbind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t ) override
			{
				using namespace so_5::disp::reuse;

				do_with_dispatcher< void, dispatcher_t >( env, m_disp_name,
					[this]( dispatcher_t & disp )
					{
						do_unbind( disp );
					} );
			}

	private:
		//! Dispatcher name to be bound to.
		const std::string m_disp_name;
	};

//
// private_dispatcher_binder_t
//

/*!
 * \since v.5.5.4
 * \brief A binder for the private %active_group dispatcher.
 */
class private_dispatcher_binder_t
	:	public so_5::rt::disp_binder_t
	,	protected binding_actions_t
	{
	public:
		explicit private_dispatcher_binder_t(
			//! A handle for private dispatcher.
			//! It is necessary to manage lifetime of the dispatcher instance.
			private_dispatcher_handle_t handle,
			//! A dispatcher instance to work with.
			dispatcher_t & instance,
			//! An active_group to be bound to.
			const std::string & group_name )
			:	binding_actions_t( group_name )
			,	m_handle( std::move( handle ) )
			,	m_instance( instance )
			{}

		virtual so_5::rt::disp_binding_activator_t
		bind_agent(
			so_5::rt::environment_t & /* env */,
			so_5::rt::agent_ref_t agent ) override
			{
				return do_bind( m_instance, std::move( agent ) );
			}

		virtual void
		unbind_agent(
			so_5::rt::environment_t & /* env */,
			so_5::rt::agent_ref_t /* agent */ ) override
			{
				do_unbind( m_instance );
			}

	private:
		//! A handle for private dispatcher.
		/*!
		 * It is necessary to manage lifetime of the dispatcher instance.
		 */
		private_dispatcher_handle_t m_handle;
		//! A dispatcher instance to work with.
		dispatcher_t & m_instance;
	};

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
		real_private_dispatcher_t()
			:	m_disp( new dispatcher_t() )
			{
				m_disp->start();
			}

		/*!
		 * Destructors shuts an instance down and waits for it.
		 */
		~real_private_dispatcher_t()
			{
				shutdown_and_wait( *m_disp );
			}

		virtual so_5::rt::disp_binder_unique_ptr_t
		binder( const std::string & group_name ) override
			{
				return so_5::rt::disp_binder_unique_ptr_t(
						new private_dispatcher_binder_t(
								private_dispatcher_handle_t( this ),
								*m_disp,
								group_name ) );
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
create_disp()
	{
		return so_5::rt::dispatcher_unique_ptr_t( new impl::dispatcher_t() );
	}

//
// create_private_disp
//
SO_5_FUNC private_dispatcher_handle_t
create_private_disp()
	{
		return private_dispatcher_handle_t(
				new impl::real_private_dispatcher_t() );
	}

//
// create_disp_binder
//
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	const std::string & disp_name,
	const std::string & group_name )
	{
		return so_5::rt::disp_binder_unique_ptr_t(
			new impl::disp_binder_t( disp_name, group_name ) );
	}

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */

