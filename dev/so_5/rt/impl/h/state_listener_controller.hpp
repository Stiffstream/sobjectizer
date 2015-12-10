/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of the agent state listener controller.
*/

#pragma once

#include <vector>

#include <so_5/rt/h/agent_state_listener.hpp>

namespace so_5
{

namespace impl
{

//
// state_listener_controller
//

//! An agent state listener controller.
class state_listener_controller_t
{
	public:
		state_listener_controller_t()
		{}

		~state_listener_controller_t()
		{}

		//! Handle state change.
		void
		changed(
			//! Agent which state has been changed.
			agent_t & agent,
			//! The current agent state.
			const state_t & state );

		//! Add a listener which lifetime is controlled by user.
		void
		so_add_nondestroyable_listener(
			agent_state_listener_t & state_listener );

		//! Add a listener which lifetime is controlled by agent.
		void
		so_add_destroyable_listener(
			agent_state_listener_unique_ptr_t state_listener );

	private:
		//! Add a listener.
		void
		add_listener(
			const agent_state_listener_ref_t & agent_state_listener_ref );

		//! Typedef for the listener container type.
		typedef std::vector<
				agent_state_listener_ref_t >
			agent_state_listener_array_t;

		//! Listeners.
		agent_state_listener_array_t m_listeners;
};

} /* namespace impl */

} /* namespace so_5 */
