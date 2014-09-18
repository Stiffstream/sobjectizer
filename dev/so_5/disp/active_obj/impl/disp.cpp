/*
	SObjectizer 5.
*/

#include <cstdlib>
#include <algorithm>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/exception.hpp>

#include <so_5/disp/active_obj/impl/h/disp.hpp>
#include <so_5/disp/one_thread/impl/h/disp.hpp>

namespace so_5
{

namespace disp
{

namespace active_obj
{

namespace impl
{

dispatcher_t::dispatcher_t()
{
}

dispatcher_t::~dispatcher_t()
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

	work_thread_shptr_t thread( new work_thread_t( *this ) );

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

} /* namespace impl */

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */
