/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Agent state listener definition.
*/

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/state.hpp>

#include <memory>
#include <vector>

namespace so_5
{

//
// agent_state_listener_t
//

//! Interface of the agent state listener.
/*!
 * This interface is intended for cases when the agent state switches
 * should be observed or monitored. A "state listener" is an object
 * which can be attached to an agent and the agent will inform
 * the listener about state changes. Such state listeners should implement
 * this interface.
 *
 * A unlimited count of state listeners may be attached to the agent.
 * Agent will call changed() method inside of 
 * so_5::agent_t::so_change_state() for each of them.
 *
 * \attention It is important not to change the agent state inside of changed().
 * Because this could lead to the infinite recursion.
 */
class SO_5_TYPE agent_state_listener_t
{
		agent_state_listener_t( const agent_state_listener_t & ) = delete;
		agent_state_listener_t &
		operator=( const agent_state_listener_t & ) = delete;

	public:
		agent_state_listener_t() = default;
		virtual ~agent_state_listener_t() noexcept = default;

		//! Hook method for state changes.
		/*!
		 * Agent calls this method after successful change of the state.
		 *
		 * \note
		 * Since v.5.6.0 this method is marked as noexcept.
		 */
		virtual void
		changed(
			//! Agent which state has been changed.
			agent_t & agent,
			//! New agent state.
			const state_t & state ) noexcept = 0;
};

//! Typedef for the agent_state_listener autopointer.
using agent_state_listener_unique_ptr_t =
		std::unique_ptr< agent_state_listener_t >;

namespace impl
{

//
// internal_state_listener_unique_ptr_t
//
//! Name of unique-ptr to be used inside SObjectizer.
/*!
 * Internal unique-ptr to agent_state_listener_t should have a custom
 * deleter. It allows to use the same unique_ptr for destroyable and
 * non-destroyable listeners.
 *
 * \since
 * v.5.6.0
 */
using internal_state_listener_unique_ptr_t =
		std::unique_ptr<
				agent_state_listener_t,
				void (*)(agent_state_listener_t *) >;

//
// state_listener_controller_t
//
class state_listener_controller_t final
	{
		using container_t = std::vector< internal_state_listener_unique_ptr_t >;

		//! All listeners.
		container_t m_listeners;

		static void
		actual_deleter( agent_state_listener_t * listener )
			{
				delete listener;
			}

		static void
		noop_deleter( agent_state_listener_t * )
			{}

	public :
		static auto
		wrap_nondestroyable(
			agent_state_listener_t & listener )
			{
				return internal_state_listener_unique_ptr_t{
						&listener,
						&state_listener_controller_t::noop_deleter
				};
			}

		static auto
		wrap_destroyable(
			agent_state_listener_unique_ptr_t listener )
			{
				return internal_state_listener_unique_ptr_t{
						listener.release(),
						&state_listener_controller_t::actual_deleter
				};
			}

		//! Add a new listener.
		/*!
		 * Actual listener should be wrapped into
		 * internal_state_listener_unique_ptr_t by using
		 * wrap_nondestroyable() or wrap_destroyable().
		 */
		void
		add(
			internal_state_listener_unique_ptr_t listener )
			{
				m_listeners.push_back( std::move(listener) );
			}

		//! Handle state change.
		void
		changed(
			//! Agent which state has been changed.
			agent_t & agent,
			//! The current agent state.
			const state_t & state ) noexcept
			{
				for( auto & l : m_listeners )
					l->changed( agent, state );
			}
	};

} /* namespace impl */

} /* namespace so_5 */

