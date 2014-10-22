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
	m_agent_threads[ &agent ] = thread;

	return thread->get_agent_binding();
}

void
dispatcher_t::destroy_thread_for_agent( const so_5::rt::agent_t & agent )
{
	std::lock_guard< std::mutex > lock( m_lock );

	if( !m_shutdown_started )
	{
		auto it = m_agent_threads.find( &agent );

		if( m_agent_threads.end() != it )
		{
			it->second->shutdown();
			it->second->wait();
			m_agent_threads.erase( it );
		}
	}
}

//
// disp_binder_t
//

//! Agent dispatcher binder.
class disp_binder_t
	:
		public so_5::rt::disp_binder_t
{
	public:
		disp_binder_t(
			const std::string & disp_name )
			:	m_disp_name( disp_name )
		{}

		virtual so_5::rt::disp_binding_activator_t
		bind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent_ref )
		{
			using so_5::rt::disp_binding_activator_t;
			using namespace so_5::disp::reuse;

			return do_with_dispatcher< disp_binding_activator_t, dispatcher_t >(
				env,
				m_disp_name,
				[this, agent_ref]( dispatcher_t & disp ) -> disp_binding_activator_t
				{
					auto ctx = disp.create_thread_for_agent( *agent_ref );

					try
					{
						disp_binding_activator_t activator =
							[agent_ref, ctx]() {
								agent_ref->so_bind_to_dispatcher( *ctx );
							};

						return activator;
					}
					catch( ... )
					{
						// Dispatcher for the agent should be removed.
						disp.destroy_thread_for_agent( *agent_ref );
						throw;
					}
				} );
		}

		virtual void
		unbind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent_ref )
		{
			using namespace so_5::disp::reuse;

			do_with_dispatcher< void, dispatcher_t >( env, m_disp_name,
				[this, agent_ref]( dispatcher_t & disp )
				{
					disp.destroy_thread_for_agent( *agent_ref );
				} );
		}

	private:
		//! Name of the dispatcher to be bound to.
		const std::string m_disp_name;
};

} /* namespace impl */

SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp()
{
	return so_5::rt::dispatcher_unique_ptr_t(
		new impl::dispatcher_t );
}

SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder( const std::string & disp_name )
{
	return so_5::rt::disp_binder_unique_ptr_t( 
		new impl::disp_binder_t( disp_name ) );
}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */
