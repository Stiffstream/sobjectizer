/*
	SObjectizer 5.
*/

#include <cstdlib>
#include <algorithm>
#include <mutex>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/exception.hpp>

#include <so_5/disp/active_group/impl/h/disp.hpp>

namespace so_5
{

namespace disp
{

namespace active_group
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

} /* namespace impl */

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */
