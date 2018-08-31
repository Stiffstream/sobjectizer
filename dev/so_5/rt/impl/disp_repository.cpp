/*
	SObjectizer 5.
*/

#include <so_5/rt/impl/h/disp_repository.hpp>

#include <so_5/h/ret_code.hpp>

#include <so_5/disp/one_thread/h/pub.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

#include <algorithm>
#include <mutex>

namespace so_5
{

namespace impl
{

//
// disp_repository_t
//

disp_repository_t::disp_repository_t(
	environment_t & env,
	named_dispatcher_map_t named_dispatcher_map,
	event_exception_logger_unique_ptr_t logger )
	:	m_env( env )
	,	m_named_dispatcher_map( std::move( named_dispatcher_map ) )
	,	m_event_exception_logger( std::move( logger ) )
	,	m_state( state_t::not_started )
{
}

dispatcher_ref_t
disp_repository_t::query_named_dispatcher(
	const std::string & disp_name )
{
	read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );
	if( state_t::started == m_state )
	{
		named_dispatcher_map_t::iterator it =
			m_named_dispatcher_map.find( disp_name );

		if( m_named_dispatcher_map.end() != it )
		{
			return it->second;
		}
	}

	return dispatcher_ref_t();
}

dispatcher_ref_t
disp_repository_t::add_dispatcher_if_not_exists(
	const std::string & disp_name,
	std::function< dispatcher_unique_ptr_t() > disp_factory )
{
	std::lock_guard< default_rw_spinlock_t > lock( m_lock );
	if( state_t::started != m_state )
		SO_5_THROW_EXCEPTION(
				rc_disp_cannot_be_added,
				"new dispatcher cannot be added when disp_repository "
				"state if not 'started'" );

	named_dispatcher_map_t::iterator it =
		m_named_dispatcher_map.find( disp_name );

	if( m_named_dispatcher_map.end() != it )
	{
		return it->second;
	}

	dispatcher_ref_t new_dispatcher = disp_factory();
	auto insert_result = m_named_dispatcher_map.emplace(
			disp_name, new_dispatcher );
	so_5::details::do_with_rollback_on_exception(
		[&] {
			new_dispatcher->set_data_sources_name_base( disp_name );
			new_dispatcher->start( m_env );
		},
		[&] { m_named_dispatcher_map.erase( insert_result.first ); } );

	return new_dispatcher;
}

void
disp_repository_t::start()
{
	std::lock_guard< default_rw_spinlock_t > lock( m_lock );
	if( state_t::not_started == m_state )
	{
		auto it = m_named_dispatcher_map.begin();
		auto it_end = m_named_dispatcher_map.end();

		for( ; it != it_end; ++it )
		{
			it->second->set_data_sources_name_base( it->first );
			it->second->start( m_env );
		}

		m_state = state_t::started;
	}
}

void
disp_repository_t::finish()
{
	{
		std::lock_guard< default_rw_spinlock_t > lock( m_lock );
		if( state_t::started == m_state )
		{
			m_state = state_t::finishing;
			send_shutdown_signal();
		}
		else
			return;
	}

	wait_for_full_shutdown();

	{
		std::lock_guard< default_rw_spinlock_t > lock( m_lock );
		m_state = state_t::not_started;
	}
}

void
disp_repository_t::install_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	if( nullptr != logger.get() )
	{
		std::lock_guard< std::mutex > lock( m_exception_logger_lock );

		event_exception_logger_unique_ptr_t old_logger;
		old_logger.swap( m_event_exception_logger );
		m_event_exception_logger.swap( logger );

		m_event_exception_logger->on_install( std::move( logger ) );
	}
}

void
disp_repository_t::call_exception_logger(
	const std::exception & event_exception,
	const std::string & coop_name )
{
	std::lock_guard< std::mutex > lock( m_exception_logger_lock );

	m_event_exception_logger->log_exception( event_exception, coop_name );
}

void
disp_repository_t::send_shutdown_signal()
{
	named_dispatcher_map_t::iterator it = m_named_dispatcher_map.begin();
	named_dispatcher_map_t::iterator it_end = m_named_dispatcher_map.end();

	for( ; it != it_end; ++it )
	{
		it->second->shutdown();
	}
}

void
disp_repository_t::wait_for_full_shutdown()
{
	named_dispatcher_map_t::iterator it = m_named_dispatcher_map.begin();
	named_dispatcher_map_t::iterator it_end = m_named_dispatcher_map.end();

	for( ; it != it_end; ++it )
	{
		it->second->wait();
	}
}

} /* namespace impl */

} /* namespace so_5 */
