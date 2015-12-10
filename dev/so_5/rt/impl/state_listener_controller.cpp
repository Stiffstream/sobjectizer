/*
	SObjectizer 5.
*/

#include <algorithm>

#include <so_5/rt/impl/h/state_listener_controller.hpp>

namespace so_5
{

namespace impl
{

namespace /* ananymous */
{

//
// destroyable_listener_item_t
//

//! A listener wrapper for the listener which lifetime is controlled by agent.
class destroyable_listener_item_t
	:
		public agent_state_listener_t
{
	public:
		destroyable_listener_item_t(
			agent_state_listener_unique_ptr_t state_listener )
			:
				m_state_listener( std::move( state_listener ) )
		{}

		virtual ~destroyable_listener_item_t()
		{}

		virtual void
		changed(
			agent_t & agent,
			const state_t & state )
		{
			m_state_listener->changed( agent, state );
		}

	private:
		agent_state_listener_unique_ptr_t m_state_listener;
};

//
// nondestroyable_listener_item_t
//

//! A listener wrapper for the listener which lifetime is controlled by user.
class nondestroyable_listener_item_t
	:
		public agent_state_listener_t
{
	public:
		nondestroyable_listener_item_t(
			agent_state_listener_t & state_listener )
			:
				m_state_listener( state_listener )
		{}

		virtual ~nondestroyable_listener_item_t()
		{}

		virtual void
		changed(
			agent_t & agent,
			const state_t & state )
		{
			m_state_listener.changed( agent, state );
		}

	private:
		agent_state_listener_t & m_state_listener;
};

} /* ananymous namespace */

void
state_listener_controller_t::changed(
	agent_t & agent,
	const state_t & state )
{
	std::for_each(
		m_listeners.begin(),
		m_listeners.end(),
		[&agent, &state]( agent_state_listener_ref_t & listener ) {
			listener->changed( agent, state );
		} );
}

void
state_listener_controller_t::so_add_nondestroyable_listener(
	agent_state_listener_t & state_listener )
{
	add_listener( agent_state_listener_ref_t(
		new nondestroyable_listener_item_t( state_listener ) ) );
}

void
state_listener_controller_t::so_add_destroyable_listener(
	agent_state_listener_unique_ptr_t state_listener )
{
	add_listener( agent_state_listener_ref_t(
		new destroyable_listener_item_t(
			std::move( state_listener ) ) ) );
}

inline void
state_listener_controller_t::add_listener(
	const agent_state_listener_ref_t & agent_state_listener_ref )
{
	m_listeners.push_back( agent_state_listener_ref );
}

} /* namespace impl */

} /* namespace so_5 */
