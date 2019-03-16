/*
	SObjectizer 5.
*/

#include <so_5/coop.hpp>

#include <so_5/impl/internal_env_iface.hpp>
#include <so_5/impl/internal_agent_iface.hpp>
#include <so_5/impl/agent_ptr_compare.hpp>

#include <so_5/details/abort_on_fatal_error.hpp>

#include <so_5/exception.hpp>
#include <so_5/environment.hpp>

#include <exception>
#include <algorithm>

namespace so_5
{

//
// coop_reg_notificators_container_t
//
void
coop_reg_notificators_container_t::call_all(
	environment_t & env,
	const coop_handle_t & coop ) const noexcept
	{
		for( auto & n : m_notificators )
			n( env, coop );
	}

//
// coop_dereg_notificators_container_t
//
void
coop_dereg_notificators_container_t::call_all(
	environment_t & env,
	const coop_handle_t & coop,
	const coop_dereg_reason_t & reason ) const noexcept
	{
		for( auto & n : m_notificators )
			n( env, coop, reason );
	}

namespace impl
{

//
// coop_impl_t
//
void
coop_impl_t::destroy_content(
	coop_t & coop ) noexcept
	{
		// Initiate deleting of agents by hand to guarantee that
		// agents will be destroyed before return from coop_t
		// destructor.
		//
		// NOTE: because agents are stored here by smart references
		// for some agents this operation will lead only to reference
		// counter descrement. Not to deletion of agent.
		coop.m_agent_array.clear();

		// Now all user resources should be destroyed.
		for( auto & d : coop.m_resource_deleters )
			d();
	}

void
coop_impl_t::do_add_agent(
	coop_t & coop,
	agent_ref_t agent_ref )
	{
		coop.m_agent_array.emplace_back(
				std::move(agent_ref), coop.m_coop_disp_binder );
	}

void
coop_impl_t::do_add_agent(
	coop_t & coop,
	agent_ref_t agent_ref,
	disp_binder_shptr_t disp_binder )
	{
		coop.m_agent_array.emplace_back(
				std::move(agent_ref), std::move(disp_binder) );
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
		N notificator )
	{
		if( !to )
		{
			to = intrusive_ptr_t< C >( new C() );
		}

		to->add( std::move(notificator) );
	}

} /* namespace anonymous */

void
coop_impl_t::add_reg_notificator(
	coop_t & coop,
	coop_reg_notificator_t notificator )
	{
		do_add_notificator_to(
				coop.m_reg_notificators,
				std::move(notificator) );
	}

void
coop_impl_t::add_dereg_notificator(
	coop_t & coop,
	coop_dereg_notificator_t notificator )
	{
		do_add_notificator_to(
				coop.m_dereg_notificators,
				std::move(notificator) );
	}

SO_5_NODISCARD
exception_reaction_t
coop_impl_t::exception_reaction(
	const coop_t & coop ) noexcept
	{
		if( inherit_exception_reaction == coop.m_exception_reaction )
			{
				const auto parent = coop.m_parent.to_shptr_noexcept();
				if( parent )
					return parent->exception_reaction();
				else
					return coop.environment().exception_reaction();
			}

		return coop.m_exception_reaction;
	}

void
coop_impl_t::do_decrement_reference_count(
	coop_t & coop )
	{
		// If it is the last working agent then Environment should be
		// informed that the cooperation is ready to be deregistered.
		if( 0 == --coop.m_reference_count )
		{
			impl::internal_env_iface_t{ coop.m_env.get() }
					.ready_to_deregister_notify( coop.shared_from_this() );
		}
	}

class coop_impl_t::registration_performer_t
	{
		coop_t & m_coop;

		void
		perform_actions_without_rollback_on_exception()
			{
				reorder_agents_with_respect_to_priorities();
				bind_agents_to_coop();
				preallocate_disp_resources();
			}

		void
		perform_actions_with_rollback_on_exception()
			{
				so_5::details::do_with_rollback_on_exception( [this] {
						define_all_agents();

						// Coop's lock should be acquired before notification
						// of the parent coop.
						std::lock_guard lock{ m_coop.m_lock };
						make_relation_with_parent_coop();

						// These actions shouldn't throw.
						details::invoke_noexcept_code( [&] {
							// This operation shouldn't throw because dispatchers
							// allocated resources for agents.
							//
							// But it is possible that an exception will be throw
							// during an attempt to send evt_start message to agents.
							// In that case it is simpler to call std::terminate().
							bind_agents_to_disp();

							// Cooperation should assume that it is registered now.
							m_coop.m_registration_status =
									coop_t::registration_status_t::coop_registered;

							// Increment reference count to reflect that cooperation
							// is registered. This is necessary in v.5.5.12 to prevent
							// automatic deregistration of the cooperation right after
							// finish of registration process for empty cooperation.
							m_coop.increment_usage_count();
						} );
					},
					[this] {
						// NOTE: we use the fact that actual binding of agents to
						// dispatchers can't throw. It means that exception was thrown
						// at earlier stages (in define_all_agents() or
						// make_relation_with_parent_coop()).
						deallocate_disp_resources();
					} );
			}

		void
		reorder_agents_with_respect_to_priorities() noexcept
			{
				std::sort(
						std::begin(m_coop.m_agent_array),
						std::end(m_coop.m_agent_array),
						[]( const auto & a, const auto & b ) noexcept {
							return special_agent_ptr_compare(
									*a.m_agent_ref, *b.m_agent_ref );
						} );
			}

		void
		bind_agents_to_coop()
			{
				for( auto & i : m_coop.m_agent_array )
					internal_agent_iface_t{ *i.m_agent_ref }.bind_to_coop( m_coop );
			}

		void
		preallocate_disp_resources()
			{
				// In case of an exception we should undo preallocation only for
				// those agents for which preallocation was successful.
				coop_t::agent_array_t::iterator it;
				try
					{
						for( it = m_coop.m_agent_array.begin();
								it != m_coop.m_agent_array.end();
								++it )
							{
								it->m_binder->preallocate_resources(
										*(it->m_agent_ref) );
							}
					}
				catch( const std::exception & x )
					{
						// All preallocated resources should be returned back.
						for( auto it2 = m_coop.m_agent_array.begin();
								it2 != it;
								++it2 )
							{
								it2->m_binder->undo_preallocation(
										*(it2->m_agent_ref) );
							}

						SO_5_THROW_EXCEPTION(
								rc_agent_to_disp_binding_failed,
								std::string{
										"an exception during the first stage of "
										"binding agent to the dispatcher, exception: " }
								+ x.what() );
					}
			}

		void
		define_all_agents()
			{
				try
					{
						for( auto & info : m_coop.m_agent_array )
							internal_agent_iface_t{ *info.m_agent_ref }
									.initiate_agent_definition();
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
				catch( ... )
					{
						SO_5_THROW_EXCEPTION(
							rc_coop_define_agent_failed,
							"exception of unknown type has been thrown in "
							"so_define_agent()" );
					}
			}

		void
		make_relation_with_parent_coop()
			{
				m_coop.m_parent.to_shptr()->add_child(
						m_coop.shared_from_this() );
			}

		void
		bind_agents_to_disp() noexcept
			{
				for( auto & info : m_coop.m_agent_array )
					info.m_binder->bind( *info.m_agent_ref );
			}

		void
		deallocate_disp_resources() noexcept
			{
				for( auto & info : m_coop.m_agent_array )
					info.m_binder->undo_preallocation( *(info.m_agent_ref) );
			}

	public :
		explicit registration_performer_t( coop_t & coop ) noexcept
			:	m_coop{ coop }
			{}

		void
		perform()
			{
				// On first phase we perform actions that don't require
				// any rollback on exception.
				perform_actions_without_rollback_on_exception();

				// Then we should perform some actions that require some
				// rollback in the case of an exception.
				perform_actions_with_rollback_on_exception();
			}
	};

void
coop_impl_t::do_registration_specific_actions( coop_t & coop )
	{
		registration_performer_t{ coop }.perform();
	}

void
coop_impl_t::do_add_child(
	coop_t & parent,
	coop_shptr_t child )
	{
		// Modification of parent-child relationship must be performed
		// on locked object.
		std::lock_guard lock{ parent.m_lock };

		// A new coop can't be added as a child if coop is being deregistered.
		if( coop_t::registration_status_t::coop_registered !=
				parent.m_registration_status )
			SO_5_THROW_EXCEPTION(
					//FIXME: this error code should be added.
					rc_coop_is_not_in_registered_state,
					"add_child() can be processed only when coop is registered" );

		// New child will be inserted to the head of children list.
		if( parent.m_first_child )
			parent.m_first_child->m_prev_sibling = child;

		child->m_next_sibling = std::move(parent.m_first_child);

		parent.m_first_child = std::move(child);

		// Count of users on this coop is incremented.
		parent.increment_usage_count();
	}

} /* namespace impl */

#if 0
coop_t::coop_t(
	nonempty_name_t name,
	disp_binder_shptr_t coop_disp_binder,
	environment_t & env )
	:	m_coop_name( name.giveout_value() )
	,	m_coop_disp_binder( std::move(coop_disp_binder) )
	,	m_env( env )
	,	m_reference_count( 0l )
	,	m_parent_coop_ptr( nullptr )
	,	m_registration_status( registration_status_t::coop_not_registered )
	,	m_exception_reaction( inherit_exception_reaction )
{
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
	for( auto & info : m_agent_array )
	{
		info.m_agent_ref->bind_to_coop( *this );
	}
}

void
coop_t::define_all_agents()
{
	for( auto & info : m_agent_array )
	{
		info.m_agent_ref->so_initiate_agent_definition();
	}
}

void
coop_t::bind_agents_to_disp()
{
	// All the following actions must be performed on locked m_binding_lock.
	// It prevents evt_start event from execution until all agents will be
	// bound to its dispatchers.
	std::lock_guard< std::mutex > binding_lock{ m_binding_lock };

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
				it->m_binder->preallocate_resources( *(it->m_agent_ref) );
			}
		}
		catch( const std::exception & x )
		{
			// All preallocated resources should be returned back.
			for( auto it2 = m_agent_array.begin(); it2 != it; ++it2 )
				it2->m_binder->undo_preallocation( *(it2->m_agent_ref) );

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
	for( auto & info : m_agent_array )
		info.m_binder->bind( *(info.m_agent_ref) );
}

void
coop_t::shutdown_all_agents()
{
	try
	{
		for( auto info : m_agent_array )
		{
			info.m_agent_ref->shutdown_agent();
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
	for( auto & info : m_agent_array )
		info.m_binder->unbind( *(info.m_agent_ref) );

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
	for( auto & d : m_resource_deleters )
		d();
}

const coop_dereg_reason_t &
coop_t::dereg_reason() const
{
	return m_dereg_reason;
}
#endif

} /* namespace so_5 */

