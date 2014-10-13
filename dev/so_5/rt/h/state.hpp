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
		 *
		 * \note This method must be used for messages which are
		 * sent to agent's direct mbox.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::rt::agent_t
			{
				const so_5::rt::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event( [=]( const msg_reconfig & evt ) { ... } );
					st_normal.event( &my_agent::evt_shutdown );
					...
				}
			};
			\endcode
		 */
		template< typename... ARGS >
		const state_t &
		event( ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for messages which are
		 * sent to \a from message-box.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::rt::agent_t
			{
				const so_5::rt::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event( m_owner, [=]( const msg_reconfig & evt ) { ... } );
					st_normal.event( m_owner, &my_agent::evt_shutdown );
					...
				}
			private :
				so_5::rt::mbox_ref_t m_owner;
			};
			\endcode
		 */
		template< typename... ARGS >
		const state_t &
		event( mbox_ref_t & from, ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for messages which are
		 * sent to \a from message-box.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::rt::agent_t
			{
				const so_5::rt::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event( m_owner, [=]( const msg_reconfig & evt ) { ... } );
					st_normal.event( m_owner, &my_agent::evt_shutdown );
					...
				}
			private :
				const so_5::rt::mbox_ref_t m_owner;
			};
			\endcode
		 */
		template< typename... ARGS >
		const state_t &
		event( const mbox_ref_t & from, ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for signal subscriptions.
		 * \note This method must be used for messages which are
		 * sent to agent's direct mbox.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::rt::agent_t
			{
				const so_5::rt::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event< msg_reconfig >( [=] { ... } );
					st_normal.event< msg_shutdown >( &my_agent::evt_shutdown );
					...
				}
			};
			\endcode
		 */
		template< typename SIGNAL, typename... ARGS >
		const state_t &
		event( ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for signal subscriptions.
		 * \note This method must be used for messages which are
		 * sent to \a from message-box.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::rt::agent_t
			{
				const so_5::rt::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event< msg_reconfig >( m_owner, [=] { ... } );
					st_normal.event< msg_shutdown >( m_owner, &my_agent::evt_shutdown );
					...
				}
			private :
				so_5::rt::mbox_ref_t m_owner;
			};
			\endcode
		 */
		template< typename SIGNAL, typename... ARGS >
		const state_t &
		event( mbox_ref_t & from, ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for signal subscriptions.
		 * \note This method must be used for messages which are
		 * sent to \a from message-box.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::rt::agent_t
			{
				const so_5::rt::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event< msg_reconfig >( m_owner, [=] { ... } );
					st_normal.event< msg_shutdown >( m_owner, &my_agent::evt_shutdown );
					...
				}
			private :
				const so_5::rt::mbox_ref_t m_owner;
			};
			\endcode
		 */
		template< typename SIGNAL, typename... ARGS >
		const state_t &
		event( const mbox_ref_t & from, ARGS&&... args ) const;

	private:
		//! Owner of this state.
		agent_t * const m_target_agent;

		//! State name.
		std::string m_state_name;

		inline const state_t *
		self_ptr() const { return this; }

		/*!
		 * \since v.5.5.1
		 * \brief A helper for handle-methods implementation.
		 */
		template< typename... ARGS >
		const state_t &
		subscribe_message_handler(
			const mbox_ref_t & from,
			ARGS&&... args ) const;

		/*!
		 * \since v.5.5.1
		 * \brief A helper for handle-methods implementation.
		 */
		template< typename SIGNAL, typename... ARGS >
		const state_t &
		subscribe_signal_handler(
			const mbox_ref_t & from,
			ARGS&&... args ) const;
};

} /* namespace rt */

} /* namespace so_5 */

#endif
