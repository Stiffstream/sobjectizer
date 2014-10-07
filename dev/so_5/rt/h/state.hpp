/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A class for the agent state definition.
*/

#if !defined( _SO_5__RT__STATE_HPP_ )
#define _SO_5__RT__STATE_HPP_

#include <string>
#include <map>
#include <set>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/mbox_ref_fwd.hpp>

namespace so_5
{

namespace rt
{

class state_t;
class agent_t;

//
// state_t
//

//! Class for the representing agent state.
class SO_5_TYPE state_t
{
		state_t( const state_t & ) = delete;
		state_t & operator =( const state_t & ) = delete;

	public:
		/*!
		 * \brief Constructor without user specified name.
		 *
		 * A name for the state will be generated automatically.
		 */
		explicit state_t(
			agent_t * agent );
		/*!
		 * \brief Full constructor.
		 */
		state_t(
			agent_t * agent,
			std::string state_name );
		/*!
		 * \since v.5.4.0
		 * \brief Move constructor.
		 */
		state_t( state_t && other );

		virtual ~state_t();

		bool
		operator == ( const state_t & state ) const;

		//! Get textual name of the state.
		const std::string &
		query_name() const;

		//! Does agent owner of this state?
		bool
		is_target( const agent_t * agent ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Switch agent to that state.
		 */
		void
		activate() const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 */
		template< typename... ARGS >
		const state_t &
		handle( ARGS&&... args ) const;

		template< typename... ARGS >
		const state_t &
		handle( mbox_ref_t & from, ARGS&&... args ) const;

		template< typename... ARGS >
		const state_t &
		handle( const mbox_ref_t & from, ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 */
		template< typename SIGNAL, typename... ARGS >
		const state_t &
		handle( ARGS&&... args ) const;

		template< typename SIGNAL, typename... ARGS >
		const state_t &
		handle( mbox_ref_t & from, ARGS&&... args ) const;

		template< typename SIGNAL, typename... ARGS >
		const state_t &
		handle( const mbox_ref_t & from, ARGS&&... args ) const;

	private:
		//! Owner of this state.
		agent_t * const m_target_agent;

		//! State name.
		std::string m_state_name;

		inline const state_t *
		self_ptr() const { return this; }

		template< typename... ARGS >
		const state_t &
		subscribe_message_handler(
			const mbox_ref_t & from,
			ARGS&&... args ) const;

		template< typename SIGNAL, typename... ARGS >
		const state_t &
		subscribe_signal_handler(
			const mbox_ref_t & from,
			ARGS&&... args ) const;
};

} /* namespace rt */

} /* namespace so_5 */

#endif
