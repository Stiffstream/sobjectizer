/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Agent state listener definition.
*/

#pragma once

#include <memory>

#include <so_5/h/declspec.hpp>
#include <so_5/rt/h/state.hpp>

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
		virtual ~agent_state_listener_t() SO_5_NOEXCEPT = default;

		//! Hook method for state changes.
		/*!
		 * Agent calls this method after successful change of the state.
		 */
		virtual void
		changed(
			//! Agent which state has been changed.
			agent_t & agent,
			//! New agent state.
			const state_t & state ) = 0;
};

//! Typedef for the agent_state_listener autopointer.
using agent_state_listener_unique_ptr_t =
		std::unique_ptr< agent_state_listener_t >;

//! Typedef for the agent_state_listener smart pointer.
using agent_state_listener_ref_t =
		std::shared_ptr< agent_state_listener_t >;

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::agent_state_listener_t
 * instead.
 */
using agent_state_listener_t = so_5::agent_state_listener_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::agent_state_listener_unique_ptr_t instead.
 */
using agent_state_listener_unique_ptr_t = so_5::agent_state_listener_unique_ptr_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::agent_state_listener_ref_t
 * instead.
 */
using agent_state_listener_ref_t = so_5::agent_state_listener_ref_t;

} /* namespace rt */

} /* namespace so_5 */

