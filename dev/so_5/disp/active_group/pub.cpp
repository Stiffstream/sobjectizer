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

	work_thread_shptr_t thread( new work_thread_t( *this ) );
	thread->start();

	m_groups.insert(
			active_group_map_t::value_type(
					group_name,
					thread_with_refcounter_t( thread, 1 ) ) );

	return thread->get_agent_binding();
}

void
dispatcher_t::release_thread_for_group( const std::string & group_name )
{
	std::lock_guard< std::mutex > lock( m_lock );

	if( !m_shutdown_started )
	{
		auto it = m_groups.find( group_name );

		if( m_groups.end() != it && 0 == --(it->second.m_user_agent) )
		{
			it->second.m_thread->shutdown();
			it->second.m_thread->wait();
			m_groups.erase( it );
		}
	}
}

//
// disp_binder_t
//

//! Agent dispatcher binder interface.
class disp_binder_t : public so_5::rt::disp_binder_t
{
	public:
		disp_binder_t(
			const std::string & disp_name,
			const std::string & group_name )
			:	m_disp_name( disp_name )
			,	m_group_name( group_name )
		{}

		//! Bind agent to a dispatcher.
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
					auto ctx = disp.query_thread_for_group( m_group_name );

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
						disp.release_thread_for_group( m_group_name );
						throw;
					}
				} );
		}

		//! Unbind agent from the dispatcher.
		virtual void
		unbind_agent(
			so_5::rt::environment_t & env,
			so_5::rt::agent_ref_t agent_ref )
		{
			using namespace so_5::disp::reuse;

			do_with_dispatcher< void, dispatcher_t >( env, m_disp_name,
				[this]( dispatcher_t & disp )
				{
					disp.release_thread_for_group( m_group_name );
				} );
		}

	private:
		//! Dispatcher name to be bound to.
		const std::string m_disp_name;

		//! Active group name to be included in.
		const std::string m_group_name;
};

} /* namespace impl */

SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp()
{
	return so_5::rt::dispatcher_unique_ptr_t(
		new impl::dispatcher_t );
}

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
