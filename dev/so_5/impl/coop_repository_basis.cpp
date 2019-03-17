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
#include <so_5/impl/coop_repository_basis.hpp>

#include <so_5/environment.hpp>

#include <so_5/details/rollback_on_exception.hpp>
#include <so_5/details/abort_on_fatal_error.hpp>

#include <cstdlib>
#include <algorithm>

namespace so_5
{

namespace impl
{

//
// root_coop_t
//
/*!
 * \brief A type of special coop that will be used as the root coop.
 *
 * \since
 * v.5.6.0
 */
class coop_repository_basis_t::root_coop_t final : public coop_t
	{
	public :
		root_coop_t(
			coop_id_t id,
			outliving_reference_t< environment_t > env )
			:	coop_t{ id,
					coop_handle_t{}, // No parent coop.
					disp_binder_shptr_t{}, // No binder.
					env }
			{
				// Automaticaly increment usage count to prevent
				// deregistration when last child coop will be destroyed.
				coop_private_iface_t::increment_usage_count( *this );

				// Status of the coop should be changed because
				// this coop is already "registered".
				m_registration_status = registration_status_t::coop_registered;
			}

		void
		deregister_children_on_shutdown() noexcept
			{
				// List of children coop should be processed when
				// the object is locked.
				std::lock_guard lock{ m_lock };

				// Every child should be deregistered with 'shutdown' reason.
				for_each_child( []( coop_t & child ) {
						child.deregister( dereg_reason::shutdown );
					} );
			}
	};

coop_repository_basis_t::coop_repository_basis_t(
	outliving_reference_t< environment_t > environment,
	coop_listener_unique_ptr_t /*coop_listener*/ )
	:	m_env{ environment }
	{
		m_root_coop = std::make_shared< root_coop_t >(
				++m_coop_id_counter,
				m_env );
	}

SO_5_NODISCARD
coop_unique_ptr_t
coop_repository_basis_t::make_coop(
	coop_handle_t parent,
	disp_binder_shptr_t default_binder )
	{
		if( !parent )
			parent = m_root_coop->handle();

		return coop_private_iface_t::make_coop(
				++m_coop_id_counter,
				std::move(parent),
				std::move(default_binder),
				m_env );
	}

SO_5_NODISCARD
coop_handle_t
coop_repository_basis_t::register_coop(
	coop_unique_ptr_t coop_ptr )
	{
		// Phase 1: check the posibility of registration of new coop.
		// This check should be performed on locked object.
		{
			std::lock_guard lock{ m_lock };
			if( status_t::normal != m_status )
				SO_5_THROW_EXCEPTION(
						rc_coop_cant_be_registered_on_shutdown,
						"a new coop can't be registered when shutdown "
						"is in progress" );

			// A counter of registration in progress should be incremented
			// to prevent shutdown.
			++m_registrations_in_progress;
		}

		// Phase 3: finishing of registration.
		// Those actions should be performed on locked objects.
		// Phase 3 will be performed automatically just after completion
		// of Phase 2.
		const auto at_exit = details::at_scope_exit(
			[this, coop_size = coop_ptr->size()] {
				std::lock_guard lock{ m_lock };

				// Statistics should be updated.
				m_total_agents += coop_size;
				m_total_coops += 1;

				// Decrement count of registration in progress to enable
				// pending shutdown (if it is).
				--m_registrations_in_progress;

				// If it was the last registration and there is a pending
				// shutdown then we should enable shutdown procedure.
				if( 0u == m_registrations_in_progress &&
						status_t::pending_shutdown == m_status )
					{
						m_shutdown_enabled_cond.notify_one();
					}
			} );

		// Phase 2: registration by itself.
		// Can be performed on unlocked object.
		return do_registration_specific_actions( std::move(coop_ptr) );
	}

coop_repository_basis_t::final_deregistration_resul_t
coop_repository_basis_t::final_deregister_coop(
	coop_shptr_t coop )
	{
		// Count of live agent and coops should be decremented.
		const auto result = [&] {
				std::lock_guard lock{ m_lock };

				m_total_agents -= coop->size();
				--m_total_coops;

				return final_deregistration_resul_t{
						0u != m_total_coops,
						status_t::shutdown == m_status && 0u == m_total_coops
				};
			}();

		// We don't expect exceptions from the following actions.
		so_5::details::invoke_noexcept_code( [&] {
				// Coop should perform its final actions.
				coop_private_iface_t::do_final_deregistration_actions( *coop );

				// Coop's dereg notificators can be processed now.
				coop_private_iface_t::call_dereg_notificators( *coop );

				// Coop's listener should be notified.
				if( m_coop_listener )
					m_coop_listener->on_deregistered(
							m_env.get(),
							coop->handle(),
							coop_private_iface_t::dereg_reason( *coop ) );
			} );
		
		return result;
	}

void
coop_repository_basis_t::deregister_all_coop() noexcept
	{
		// Phase 1: check that shutdown is not in progress now.
		{
			std::unique_lock lock{ m_lock };

			if( !m_registrations_in_progress )
				{
					m_status = status_t::shutdown;
					// NOTE: there is no need to wait on m_shutdown_enabled_cond
					// because there is no active registration procedures.
				}
			else
				{
					// There are active registration procedures.
					// We should wait for their completion.
					m_status = status_t::pending_shutdown;
					m_shutdown_enabled_cond.wait( lock,
							[this] { return 0 == m_registrations_in_progress; } );
					m_status = status_t::shutdown;
				}
		}

		// Phase 2: deregistration of all coops.
		m_root_coop->deregister_children_on_shutdown();
	}

SO_5_NODISCARD
coop_repository_basis_t::try_switch_to_shutdown_result_t
coop_repository_basis_t::try_switch_to_shutdown()
	{
		std::lock_guard lock{ m_lock };

		if( status_t::normal == m_status )
			{
				m_status = status_t::pending_shutdown;
				return try_switch_to_shutdown_result_t::switched;
			}
		else
			return try_switch_to_shutdown_result_t::already_in_shutdown_state;
	}

environment_t &
coop_repository_basis_t::environment()
	{
		return m_env.get();
	}

environment_infrastructure_t::coop_repository_stats_t
coop_repository_basis_t::query_stats()
	{
		std::lock_guard lock{ m_lock };

		return {
				m_total_coops,
				m_total_agents,
				0u
			};
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
				coop_private_iface_t::increment_usage_count( coop );
			}
			~coop_usage_counter_guard_t()
			{
				coop_private_iface_t::decrement_usage_count( m_coop );
			}

		private :
			coop_t & m_coop;
	};

} /* namespace anonymous */

coop_handle_t
coop_repository_basis_t::do_registration_specific_actions(
	coop_unique_ptr_t coop_ptr )
{
	// Cooperation object should life to the end of this routine.
	coop_shptr_t coop{ coop_ptr.release(), coop_deleter_t() };

	// Usage counter for cooperation should be incremented right now,
	// and decremented at exit point.
	coop_usage_counter_guard_t coop_usage_quard( *coop );

	coop_private_iface_t::do_registration_specific_actions( *coop );

	auto result = coop->handle();

	// We don't expect exceptions from the following actions.
	so_5::details::invoke_noexcept_code( [&] {
			// Coop's dereg notificators can be processed now.
			coop_private_iface_t::call_reg_notificators( *coop );

			// Coop's listener should be notified.
			if( m_coop_listener )
				m_coop_listener->on_registered( m_env.get(), result );
		} );

	return result;
}

} /* namespace impl */

} /* namespace so_5 */

