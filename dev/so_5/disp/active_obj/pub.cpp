/*
	SObjectizer 5
*/

#include <so_5/disp/active_obj/h/pub.hpp>

#include <map>
#include <mutex>
#include <algorithm>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/event_queue.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>
#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5
{

namespace disp
{

namespace active_obj
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
	\brief Active objects dispatcher.
*/
class dispatcher_t
	:
		public so_5::rt::dispatcher_t
{
	public:
		dispatcher_t();

		//! \name Implemetation of so_5::rt::dispatcher methods.
		//! \{

		virtual void
		start();

		virtual void
		shutdown();

		virtual void
		wait();

		//! \}

		//! Creates a new thread for the agent specified.
		so_5::rt::event_queue_t *
		create_thread_for_agent( const so_5::rt::agent_t & agent );

		//! Destroys the thread for the agent specified.
		void
		destroy_thread_for_agent( const so_5::rt::agent_t & agent );

	private:
		//! Typedef for mapping from agents to their working threads.
		typedef std::map<
				const so_5::rt::agent_t *,
				so_5::disp::reuse::work_thread::work_thread_shptr_t >
			agent_thread_map_t;

		//! A map from agents to single thread dispatchers.
		agent_thread_map_t m_agent_threads;

		//! Shutdown flag.
		bool m_shutdown_started;

		//! This object lock.
		std::mutex m_lock;

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

	work_thread_shptr_t thread( new work_thread_t() );

	thread->start();
	try
	{
		m_agent_threads[ &agent ] = thread;
	}
	catch( ... )
	{
		shutdown_and_wait( *thread );
		throw;
	}

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
		static so_5::rt::disp_binding_activator_t
		do_bind(
			dispatcher_t & disp,
			so_5::rt::agent_ref_t agent )
			{
				auto ctx = disp.create_thread_for_agent( *agent );

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
					disp.destroy_thread_for_agent( *agent );
					throw;
				}
			}

		static void
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
class disp_binder_t
	:	public so_5::rt::disp_binder_t
	,	protected binding_actions_t
	{
	public:
		disp_binder_t(
			const std::string & disp_name )
			:	m_disp_name( disp_name )
			{}

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
					[agent]( dispatcher_t & disp )
					{
						return do_bind( disp, std::move( agent ) );
					} );
			}

		virtual void
		unbind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent ) override
			{
				using namespace so_5::disp::reuse;

				do_with_dispatcher< void, dispatcher_t >( env, m_disp_name,
					[agent]( dispatcher_t & disp )
					{
						do_unbind( disp, std::move( agent ) );
					} );
			}

	private:
		//! Name of the dispatcher to be bound to.
		const std::string m_disp_name;
	};

//
// private_dispatcher_binder_t
//

/*!
 * \since v.5.5.4
 * \brief A binder for the private %active_obj dispatcher.
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
			dispatcher_t & instance )
			:	m_handle( std::move( handle ) )
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
			so_5::rt::environment_t & /*env*/,
			so_5::rt::agent_ref_t agent ) override
			{
				do_unbind( m_instance, std::move( agent ) );
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
create_disp_binder( const std::string & disp_name )
	{
		return so_5::rt::disp_binder_unique_ptr_t( 
			new impl::disp_binder_t( disp_name ) );
	}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */

