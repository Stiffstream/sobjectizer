/*
	SObjectizer 5.
*/

#include <exception>
#include <algorithm>

#include <so_5/h/exception.hpp>

#include <so_5/rt/h/environment.hpp>

#include <so_5/rt/impl/h/internal_env_iface.hpp>

#include <so_5/rt/impl/h/agent_ptr_compare.hpp>

#include <so_5/details/h/abort_on_fatal_error.hpp>

namespace so_5
{

//
// coop_reg_notificators_container_t
//
void
coop_reg_notificators_container_t::call_all(
	environment_t & env,
	const std::string & coop_name ) const
{
	for( auto i = m_notificators.begin(); i != m_notificators.end(); ++i )
	{
		// Exceptions should not go out.
		try
		{
			(*i)( env, coop_name );
		}
		catch( const std::exception & x )
		{
			SO_5_LOG_ERROR( env, log_stream ) {
				 log_stream << "on reg_notification for coop '"
					 	<< coop_name << "' exception: " << x.what();
			}
		}
	}
}

//
// coop_dereg_notificators_container_t
//
void
coop_dereg_notificators_container_t::call_all(
	environment_t & env,
	const std::string & coop_name,
	const coop_dereg_reason_t & reason ) const
{
	for( auto i = m_notificators.begin(); i != m_notificators.end(); ++i )
	{
		// Exceptions should not go out.
		try
		{
			(*i)( env, coop_name, reason );
		}
		catch( const std::exception & x )
		{
			SO_5_LOG_ERROR( env, log_stream ) {
				 log_stream << "on dereg_notification for coop '"
					 	<< coop_name << "' exception: " << x.what();
			}
		}
	}
}

//
// coop_t
//

coop_t::~coop_t()
{
	// Initiate deleting of agents by hand to guarantee that
	// agents will be destroyed before return from coop_t
	// destructor.
	//
	// NOTE: because agents are stored here by smart references
	// for some agents this operation will lead only to reference
	// counter descrement. Not to deletion of agent.
	m_agent_array.clear();

	// Now all user resources should be destroyed.
	delete_user_resources();
}

void
coop_t::destroy( coop_t * coop )
{
	delete coop;
}

coop_t::coop_t(
	nonempty_name_t name,
	disp_binder_unique_ptr_t coop_disp_binder,
	environment_t & env )
	:	m_coop_name( name.giveout_value() )
	,	m_coop_disp_binder( std::move(coop_disp_binder) )
	,	m_env( env )
	,	m_parent_coop_ptr( nullptr )
	,	m_registration_status( registration_status_t::coop_not_registered )
	,	m_exception_reaction( inherit_exception_reaction )
{
	m_reference_count = 0l;
}

const std::string &
coop_t::query_coop_name() const
{
	return m_coop_name;
}

bool
coop_t::has_parent_coop() const
{
	return !m_parent_coop_name.empty();
}

void
coop_t::set_parent_coop_name(
	nonempty_name_t name )
{
	m_parent_coop_name = name.giveout_value();
}

const std::string &
coop_t::parent_coop_name() const
{
	if( !has_parent_coop() )
		SO_5_THROW_EXCEPTION(
				rc_coop_has_no_parent,
				query_coop_name() + ": cooperation has no parent cooperation" );

	return m_parent_coop_name;
}

namespace
{
	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Helper function for notificator addition.
	 */
	template< class C, class N >
	inline void
	do_add_notificator_to(
		intrusive_ptr_t< C > & to,
		const N & notificator )
	{
		if( !to )
		{
			to = intrusive_ptr_t< C >( new C() );
		}

		to->add( notificator );
	}

} /* namespace anonymous */

void
coop_t::add_reg_notificator(
	const coop_reg_notificator_t & notificator )
{
	do_add_notificator_to( m_reg_notificators, notificator );
}

void
coop_t::add_dereg_notificator(
	const coop_dereg_notificator_t & notificator )
{
	do_add_notificator_to( m_dereg_notificators, notificator );
}

void
coop_t::set_exception_reaction(
	exception_reaction_t value )
{
	m_exception_reaction = value;
}

exception_reaction_t
coop_t::exception_reaction() const
{
	if( inherit_exception_reaction == m_exception_reaction )
		{
			if( m_parent_coop_ptr )
				return m_parent_coop_ptr->exception_reaction();
			else
				return m_env.exception_reaction();
		}

	return m_exception_reaction;
}

void
coop_t::deregister( int reason )
{
	environment().deregister_coop( query_coop_name(), reason );
}

void
coop_t::do_add_agent(
	const agent_ref_t & agent_ref )
{
	m_agent_array.push_back(
		agent_with_disp_binder_t( agent_ref, m_coop_disp_binder ) );
}

void
coop_t::do_add_agent(
	const agent_ref_t & agent_ref,
	disp_binder_unique_ptr_t disp_binder )
{
	disp_binder_ref_t dbinder( disp_binder.release() );

	if( nullptr == dbinder.get() || nullptr == agent_ref.get() )
		throw exception_t(
			"zero ptr to agent or disp binder",
			rc_coop_has_references_to_null_agents_or_binders );

	m_agent_array.push_back(
		agent_with_disp_binder_t( agent_ref, dbinder ) );
}

void
coop_t::do_registration_specific_actions(
	coop_t * parent_coop )
{
	reorder_agents_with_respect_to_priorities();
	bind_agents_to_coop();
	define_all_agents();

	bind_agents_to_disp();

	m_parent_coop_ptr = parent_coop;
	if( m_parent_coop_ptr )
		// Parent coop should known about existence of that coop.
		m_parent_coop_ptr->m_reference_count += 1;

	// Cooperation should assume that it is registered now.
	m_registration_status = registration_status_t::coop_registered;
	// Increment reference count to reflect that cooperation is registered.
	// This is necessary in v.5.5.12 to prevent automatic deregistration
	// of the cooperation right after finish of registration process for
	// empty cooperation.
	m_reference_count += 1;
}

void
coop_t::do_deregistration_specific_actions(
	coop_dereg_reason_t dereg_reason )
{
	m_dereg_reason = std::move( dereg_reason );

	shutdown_all_agents();

	// Reference count could decremented.
	// If coop was an empty coop then this action initiates the whole
	// coop deregistration.
	decrement_usage_count();
}

void
coop_t::reorder_agents_with_respect_to_priorities()
{
	std::sort( std::begin(m_agent_array), std::end(m_agent_array),
		[]( const agent_with_disp_binder_t & a,
			const agent_with_disp_binder_t & b ) -> bool {
			return impl::special_agent_ptr_compare(
					*a.m_agent_ref, *b.m_agent_ref );
		} );
}

void
coop_t::bind_agents_to_coop()
{
	agent_array_t::iterator it = m_agent_array.begin();
	agent_array_t::iterator it_end = m_agent_array.end();

	for(; it != it_end; ++it )
	{
		it->m_agent_ref->bind_to_coop( *this );
	}
}

void
coop_t::define_all_agents()
{
	agent_array_t::iterator it = m_agent_array.begin();
	agent_array_t::iterator it_end = m_agent_array.end();

	for(; it != it_end; ++it )
	{
		it->m_agent_ref->so_initiate_agent_definition();
	}
}

void
coop_t::bind_agents_to_disp()
{
	// All the following actions must be performed on locked m_binding_lock.
	// It prevents evt_start event from execution until all agents will be
	// bound to its dispatchers.
	std::lock_guard< std::mutex > binding_lock{ m_binding_lock };

	std::vector< disp_binding_activator_t > activators;
	activators.reserve( m_agent_array.size() );

	// The first stage of binding to dispatcher:
	// allocating necessary resources for agents.
	// Exceptions on that stage will lead to simple unbinding
	// agents from dispatchers.
	{
		agent_array_t::iterator it;
		try
		{
			for( it = m_agent_array.begin(); it != m_agent_array.end(); ++it )
			{
				activators.emplace_back(
						it->m_binder->bind_agent( m_env, it->m_agent_ref ) );
			}
		}
		catch( const std::exception & x )
		{
			unbind_agents_from_disp( it );

			SO_5_THROW_EXCEPTION(
					rc_agent_to_disp_binding_failed,
					std::string( "an exception during the first stage of "
							"binding agent to the dispatcher, cooperation: '" +
							m_coop_name + "', exception: " + x.what() ) );
		}
	}

	// The second stage of binding. Activation of the allocated on
	// the first stage resources.
	// Exceptions on that stage would lead to unpredictable application
	// state. Therefore std::abort is called on exception.
	try
	{
		for( auto & a : activators )
			a();
	}
	catch( const std::exception & x )
	{
		so_5::details::abort_on_fatal_error( [&] {
			SO_5_LOG_ERROR( m_env, log_stream ) {
				log_stream << "an exception on the second stage of "
						"agents to dispatcher binding; cooperation: "
						<< m_coop_name << ", exception: " << x.what();
			}
		} );
	}
}

inline void
coop_t::unbind_agents_from_disp(
	agent_array_t::iterator it )
{
	for( auto it_begin = m_agent_array.begin(); it != it_begin; )
	{
		--it;
		it->m_binder->unbind_agent( m_env, it->m_agent_ref );
	}
}

void
coop_t::shutdown_all_agents()
{
	try
	{
		for( auto it = m_agent_array.begin(); it != m_agent_array.end(); ++it )
		{
			it->m_agent_ref->shutdown_agent();
		}
	}
	catch( const std::exception & x )
	{
		so_5::details::abort_on_fatal_error( [&] {
			SO_5_LOG_ERROR( m_env, log_stream ) {
				log_stream << "Exception during shutting cooperation agents down. "
						"Work cannot be continued. Cooperation: '"
						<< m_coop_name << "'. Exception: " << x.what();
			}
		} ); 
	}
}

void
coop_t::increment_usage_count()
{
	m_reference_count += 1;
}

void
coop_t::decrement_usage_count()
{
	// If it is the last working agent then Environment should be
	// informed that the cooperation is ready to be deregistered.
	if( 0 == --m_reference_count )
	{
		// NOTE: usage counter incremented and decremented during
		// registration process even if registration of cooperation failed.
		// So decrement_usage_count() could be called when cooperation
		// has coop_not_registered status.
		if( registration_status_t::coop_registered == m_registration_status )
		{
			m_registration_status = registration_status_t::coop_deregistering;
			impl::internal_env_iface_t{ m_env }.ready_to_deregister_notify( this );
		}
	}
}

void
coop_t::final_deregister_coop()
{
	unbind_agents_from_disp( m_agent_array.end() );

	impl::internal_env_iface_t{ m_env }.final_deregister_coop( m_coop_name );
}

coop_t *
coop_t::parent_coop_ptr() const
{
	return m_parent_coop_ptr;
}

coop_reg_notificators_container_ref_t
coop_t::reg_notificators() const
{
	return m_reg_notificators;
}

coop_dereg_notificators_container_ref_t
coop_t::dereg_notificators() const
{
	return m_dereg_notificators;
}

void
coop_t::delete_user_resources()
{
	for( auto d = m_resource_deleters.begin();
			d != m_resource_deleters.end();
			++d )
		(*d)();
}

const coop_dereg_reason_t &
coop_t::dereg_reason() const
{
	return m_dereg_reason;
}

} /* namespace so_5 */

