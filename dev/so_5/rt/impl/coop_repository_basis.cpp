/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Basic part of coop_repository functionality.
 *
 * \since
 * v.5.5.19
 */
#include <so_5/rt/impl/h/coop_repository_basis.hpp>

#include <so_5/rt/h/environment.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>
#include <so_5/details/h/abort_on_fatal_error.hpp>

#include <cstdlib>
#include <algorithm>

namespace so_5
{

namespace impl
{

namespace coop_repository_details
{

/*!
 * \since
 * v.5.2.3
 *
 * \brief Helper class for doing all actions related to
 * start of cooperation deregistration.
 *
 * This class is necessary because addition of parent-child relationship
 * in version 5.2.3. And since that version deregistration of cooperation
 * is more complex process then in previous versions.
 *
 * \attention On some stages of deregistration an exception leads to
 * call to abort().
 */
class deregistration_processor_t
{
public :
	//! Constructor.
	deregistration_processor_t(
		//! Owner of all data.
		coop_repository_basis_t & core,
		//! Name of root cooperation to be deregistered.
		const std::string & root_coop_name,
		//! Deregistration reason.
		coop_dereg_reason_t dereg_reason );

	//! Do all necessary actions.
	void
	process();

private :
	//! Owner of all data to be handled.
	coop_repository_basis_t & m_core;

	//! Name of root cooperation to be deregistered.
	const std::string & m_root_coop_name;

	//! Deregistration reason.
	/*!
	 * This value used only for parent cooperation.
	 */
	coop_dereg_reason_t m_root_coop_dereg_reason;

	//! Cooperations to be deregistered.
	std::vector< coop_ref_t > m_coops_to_dereg;

	//! Names of cooperations to be deregistered.
	std::vector< std::string > m_coops_names_to_process;

	void
	first_stage();

	bool
	has_something_to_deregister() const;

	void
	second_stage();

	coop_ref_t
	ensure_root_coop_exists() const;

	void
	collect_and_modity_coop_info(
		const coop_ref_t & root_coop );

	void
	collect_coops();

	void
	modify_registered_and_deregistered_maps();

	void
	initiate_abort_on_exception(
		const std::exception & x );
};

deregistration_processor_t::deregistration_processor_t(
	coop_repository_basis_t & core,
	const std::string & root_coop_name,
	coop_dereg_reason_t dereg_reason )
	:	m_core( core )
	,	m_root_coop_name( root_coop_name )
	,	m_root_coop_dereg_reason( std::move( dereg_reason ) )
{}

void
deregistration_processor_t::process()
{
	first_stage();

	if( has_something_to_deregister() )
		second_stage();
}

void
deregistration_processor_t::first_stage()
{
	std::lock_guard< std::mutex > lock( m_core.lock() );

	if( m_core.m_deregistered_coop.end() ==
			m_core.m_deregistered_coop.find( m_root_coop_name ) )
	{
		coop_ref_t coop = ensure_root_coop_exists();

		collect_and_modity_coop_info( coop );
	}
}

bool
deregistration_processor_t::has_something_to_deregister() const
{
	return !m_coops_to_dereg.empty();
}

void
deregistration_processor_t::second_stage()
{
	// Exceptions must lead to abort at this deregistration stage.
	try
	{
		// All cooperations should start deregistration actions.
		auto it = m_coops_to_dereg.begin();

		// The first item in that vector is a root cooperation.
		// So the actual coop_dereg_reason should be used for it.
		coop_private_iface_t::do_deregistration_specific_actions(
				**it, std::move( m_root_coop_dereg_reason ) );

		// A special value must be used for all other coops.
		for(++it; it != m_coops_to_dereg.end(); ++it )
		{
			coop_private_iface_t::do_deregistration_specific_actions(
					**it,
					coop_dereg_reason_t( dereg_reason::parent_deregistration ) );
		}
	}
	catch( const std::exception & x )
	{
		initiate_abort_on_exception( x );
	}
}

coop_ref_t
deregistration_processor_t::ensure_root_coop_exists() const
{
	// It is an error if the cooperation is not registered.
	auto it = m_core.m_registered_coop.find( m_root_coop_name );

	if( m_core.m_registered_coop.end() == it )
	{
		SO_5_THROW_EXCEPTION(
			rc_coop_has_not_found_among_registered_coop,
			"coop with name '" + m_root_coop_name +
			"' not found among registered cooperations" );
	}

	return it->second;
}

void
deregistration_processor_t::collect_and_modity_coop_info(
	const coop_ref_t & root_coop )
{
	// Exceptions must lead to abort at this deregistration stage.
	try
	{
		m_coops_to_dereg.push_back( root_coop );
		m_coops_names_to_process.push_back( m_root_coop_name );

		collect_coops();

		modify_registered_and_deregistered_maps();
	}
	catch( const std::exception & x )
	{
		initiate_abort_on_exception( x );
	}
}

void
deregistration_processor_t::collect_coops()
{
	for( size_t i = 0; i != m_coops_names_to_process.size(); ++i )
	{
		const coop_repository_basis_t::parent_child_coop_names_t relation(
				m_coops_names_to_process[ i ], std::string() );

		for( auto f = m_core.m_parent_child_relations.lower_bound( relation );
				f != m_core.m_parent_child_relations.end() &&
						f->first == m_coops_names_to_process[ i ];
				++f )
		{
			auto it = m_core.m_registered_coop.find( f->second );
			if( it != m_core.m_registered_coop.end() )
			{
				m_coops_to_dereg.push_back( it->second );
				m_coops_names_to_process.push_back( it->first );
			}
			else
			{
				// Child cooperation is not found in list of
				// registered cooperation.
				// It is not an error if the child cooperation is
				// in deregistration procedure right now.
				auto it_dereg = m_core.m_deregistered_coop.find( f->second );
				if( it_dereg == m_core.m_deregistered_coop.end() )
				{
					// This is an error: cooperation is not registered
					// and is not in deregistration phase.
					SO_5_THROW_EXCEPTION(
							rc_unexpected_error,
							f->second + ": cooperation not registered, but "
								"declared as child for: '" + f->first + "'" );
				}
			}
		}
	}
}

void
deregistration_processor_t::modify_registered_and_deregistered_maps()
{
	std::for_each(
			m_coops_names_to_process.begin(),
			m_coops_names_to_process.end(),
			[this]( const std::string & n ) {
				auto it = m_core.m_registered_coop.find( n );
				m_core.m_deregistered_coop.insert( *it );
				m_core.m_registered_coop.erase( it );
			} );
}

void
deregistration_processor_t::initiate_abort_on_exception(
	const std::exception & x )
{
	so_5::details::abort_on_fatal_error( [&] {
		SO_5_LOG_ERROR( m_core.environment(), log_stream )
		{
			log_stream << "Exception during cooperation deregistration. "
					"Work cannot be continued. Cooperation: '"
					<< m_root_coop_name << "'. Exception: '"
					<< x.what() << "'";
		}
	} );
}

} /* namespace agent_core_details */

coop_repository_basis_t::coop_repository_basis_t(
	environment_t & so_environment,
	coop_listener_unique_ptr_t coop_listener )
	:	m_so_environment( so_environment )
	,	m_deregistration_started( false )
	,	m_total_agent_count{ 0 }
	,	m_coop_listener( std::move( coop_listener ) )
{
}

namespace
{
	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Special guard to increment and decrement cooperation
	 * usage counters.
	 */
	class coop_usage_counter_guard_t
	{
		public :
			coop_usage_counter_guard_t( coop_t & coop )
				:	m_coop( coop )
			{
				coop_t::increment_usage_count( coop );
			}
			~coop_usage_counter_guard_t()
			{
				coop_t::decrement_usage_count( m_coop );
			}

		private :
			coop_t & m_coop;
	};

} /* namespace anonymous */

void
coop_repository_basis_t::register_coop(
	coop_unique_ptr_t coop_ptr )
{
	/*!
	 * \note For some important details see
	 * coop_t::increment_usage_count().
	 */

	if( nullptr == coop_ptr.get() )
		SO_5_THROW_EXCEPTION(
			rc_zero_ptr_to_coop,
			"zero ptr to coop passed" );

	// Cooperation object should life to the end of this routine.
	coop_ref_t coop_ref( coop_ptr.release(), coop_deleter_t() );

	// Usage counter for cooperation should be incremented right now,
	// and decremented at exit point.
	coop_usage_counter_guard_t coop_usage_quard( *coop_ref );

	try
	{
		// All the following actions should be taken under the lock.
		std::lock_guard< std::mutex > lock( m_coop_operations_lock );

		if( m_deregistration_started )
			SO_5_THROW_EXCEPTION(
					rc_unable_to_register_coop_during_shutdown,
					coop_ref->query_coop_name() +
					": a new cooperation cannot be started during "
					"environment shutdown" );

		// Name should be unique.
		ensure_new_coop_name_unique( coop_ref->query_coop_name() );
		// Process parent coop.
		coop_t * parent = find_parent_coop_if_necessary( *coop_ref );

		next_coop_reg_step__update_registered_coop_map(
				coop_ref,
				parent );
	}
	catch( const exception_t & )
	{
		throw;
	}
	catch( const std::exception & ex )
	{
		SO_5_THROW_EXCEPTION(
			rc_coop_define_agent_failed,
			ex.what() );
	}

	do_coop_reg_notification_if_necessary(
		coop_ref->query_coop_name(),
		coop_private_iface_t::reg_notificators( *coop_ref ) );
}

void
coop_repository_basis_t::deregister_coop(
	nonempty_name_t name,
	coop_dereg_reason_t dereg_reason )
{
	coop_repository_details::deregistration_processor_t processor(
			*this,
			name.query_name(),
			std::move( dereg_reason ) );

	processor.process();
}

coop_repository_basis_t::final_deregistration_resul_t
coop_repository_basis_t::final_deregister_coop(
	std::string coop_name )
{
	final_remove_result_t remove_result;

	bool has_live_coops = false;
	bool need_signal_dereg_finished = false;
	{
		std::lock_guard< std::mutex > lock( m_coop_operations_lock );

		remove_result = finaly_remove_cooperation_info( coop_name );

		// If we are inside shutdown process and this is the last
		// cooperation then a special flag should be set.
		need_signal_dereg_finished =
			m_deregistration_started && m_deregistered_coop.empty();

		has_live_coops = !m_registered_coop.empty() ||
				!m_deregistered_coop.empty();
	}

	// Cooperation must be destroyed.
	remove_result.m_coop.reset();

	do_coop_dereg_notification_if_necessary(
			coop_name,
			remove_result.m_notifications );

	return { has_live_coops, need_signal_dereg_finished };
}

std::size_t
coop_repository_basis_t::deregister_all_coop() SO_5_NOEXCEPT
{
	// Because VC++ 12.0 doesn't support noexcept we use invoke_noexcept_code.
	return so_5::details::invoke_noexcept_code( [this] {
		std::lock_guard< std::mutex > lock( m_coop_operations_lock );

		for( auto & info : m_registered_coop )
			coop_private_iface_t::do_deregistration_specific_actions(
					*(info.second),
					coop_dereg_reason_t( dereg_reason::shutdown ) );
				
		m_deregistered_coop.insert(
			m_registered_coop.begin(),
			m_registered_coop.end() );

		m_registered_coop.clear();
		m_deregistration_started = true;

		return m_deregistered_coop.size();
	} );
}

coop_repository_basis_t::initiate_deregistration_result_t
coop_repository_basis_t::initiate_deregistration()
{
	initiate_deregistration_result_t result =
			initiate_deregistration_result_t::already_in_progress;
	{
		std::lock_guard< std::mutex > lock( m_coop_operations_lock );

		if( !m_deregistration_started )
		{
			m_deregistration_started = true;
			result = initiate_deregistration_result_t::initiated_first_time;
		}
	}

	return result;
}

environment_t &
coop_repository_basis_t::environment()
{
	return m_so_environment;
}

environment_infrastructure_t::coop_repository_stats_t
coop_repository_basis_t::query_stats()
{
	std::unique_lock< std::mutex > lock( m_coop_operations_lock );

	return {
			m_registered_coop.size(),
			m_deregistered_coop.size(),
			m_total_agent_count,
			0u
		};
}

void
coop_repository_basis_t::ensure_new_coop_name_unique(
	const std::string & coop_name ) const
{
	if( m_registered_coop.end() != m_registered_coop.find( coop_name ) ||
		m_deregistered_coop.end() != m_deregistered_coop.find( coop_name ) )
	{
		SO_5_THROW_EXCEPTION(
			rc_coop_with_specified_name_is_already_registered,
			"coop with name \"" + coop_name + "\" is already registered" );
	}
}

coop_t *
coop_repository_basis_t::find_parent_coop_if_necessary(
	const coop_t & coop_to_be_registered ) const
{
	if( coop_to_be_registered.has_parent_coop() )
	{
		auto it = m_registered_coop.find(
				coop_to_be_registered.parent_coop_name() );
		if( m_registered_coop.end() == it )
		{
			SO_5_THROW_EXCEPTION(
				rc_parent_coop_not_found,
				"parent coop with name \"" +
					coop_to_be_registered.parent_coop_name() +
					"\" is not registered" );
		}

		return it->second.get();
	}

	return nullptr;
}

void
coop_repository_basis_t::next_coop_reg_step__update_registered_coop_map(
	const coop_ref_t & coop_ref,
	coop_t * parent_coop_ptr )
{
	m_registered_coop[ coop_ref->query_coop_name() ] = coop_ref;
	m_total_agent_count += coop_ref->query_agent_count();

	// In case of error cooperation info should be removed
	// from m_registered_coop.
	so_5::details::do_with_rollback_on_exception(
		[&] {
			next_coop_reg_step__parent_child_relation(
					coop_ref,
					parent_coop_ptr );
		},
		[&] {
			m_total_agent_count -= coop_ref->query_agent_count();
			m_registered_coop.erase( coop_ref->query_coop_name() );
		} );
}

void
coop_repository_basis_t::next_coop_reg_step__parent_child_relation(
	const coop_ref_t & coop_ref,
	coop_t * parent_coop_ptr )
{
	auto do_actions = [&] {
			coop_private_iface_t::do_registration_specific_actions(
				*coop_ref, parent_coop_ptr );
		};

	if( parent_coop_ptr )
	{
		const parent_child_coop_names_t names{
				parent_coop_ptr->query_coop_name(),
				coop_ref->query_coop_name() };

		m_parent_child_relations.insert( names );

		// In case of error cooperation relation info should be removed
		// from m_parent_child_relations.
		so_5::details::do_with_rollback_on_exception(
			[&] { do_actions(); },
			[&] { m_parent_child_relations.erase( names ); } );
	}
	else
		// It is a very simple case. There is no need for additional
		// actions and rollback on exceptions.
		do_actions();
}

coop_repository_basis_t::final_remove_result_t
coop_repository_basis_t::finaly_remove_cooperation_info(
	const std::string & coop_name )
{
	auto it = m_deregistered_coop.find( coop_name );
	if( it != m_deregistered_coop.end() )
	{
		coop_ref_t removed_coop = it->second;
		m_deregistered_coop.erase( it );
		m_total_agent_count -= removed_coop->query_agent_count();

		coop_t * parent =
				coop_private_iface_t::parent_coop_ptr( *removed_coop );
		if( parent )
		{
			m_parent_child_relations.erase(
					parent_child_coop_names_t(
							parent->query_coop_name(),
							coop_name ) );

			coop_t::decrement_usage_count( *parent );
		}

		return final_remove_result_t{
				removed_coop,
				info_for_dereg_notification_t{
						coop_private_iface_t::dereg_reason(
								*removed_coop ),
						coop_private_iface_t::dereg_notificators(
								*removed_coop ) } };
	}
	else
		return final_remove_result_t{};
}

void
coop_repository_basis_t::do_coop_reg_notification_if_necessary(
	const std::string & coop_name,
	const coop_reg_notificators_container_ref_t & notificators ) const
{
	if( m_coop_listener.get() )
		m_coop_listener->on_registered( m_so_environment, coop_name );

	if( notificators )
		notificators->call_all( m_so_environment, coop_name );
}

void
coop_repository_basis_t::do_coop_dereg_notification_if_necessary(
	const std::string & coop_name,
	const info_for_dereg_notification_t & notification_info ) const
{
	if( m_coop_listener.get() )
		m_coop_listener->on_deregistered(
				m_so_environment,
				coop_name,
				notification_info.m_reason );

	if( notification_info.m_notificators )
		notification_info.m_notificators->call_all(
				m_so_environment,
				coop_name,
				notification_info.m_reason );
}

} /* namespace impl */

} /* namespace so_5 */

