/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Agent state listener definition.
*/

#if !defined( _SO_5__RT__AGENT_STATE_LISTENER_HPP_ )
#define _SO_5__RT__AGENT_STATE_LISTENER_HPP_

#include <memory>

#include <so_5/h/declspec.hpp>
#include <so_5/rt/h/state.hpp>

namespace so_5
{

namespace rt
{

class state_t;

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
 * so_5::rt::agent_t::so_change_state() for each of them.
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
		agent_state_listener_t();
		virtual ~agent_state_listener_t();

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
typedef std::unique_ptr< agent_state_listener_t >
	agent_state_listener_unique_ptr_t;

//! Typedef for the agent_state_listener smart pointer.
typedef std::shared_ptr< agent_state_listener_t >
	agent_state_listener_ref_t;

} /* namespace rt */

} /* namespace so_5 */

#endif

