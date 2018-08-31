/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A class for the agent state definition.
*/

#pragma once

#include <array>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <chrono>

#include <so_5/h/compiler_features.hpp>
#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/mbox_fwd.hpp>
#include <so_5/rt/h/fwd.hpp>

#include <so_5/rt/h/message_handler_format_detector.hpp>

namespace so_5
{

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

//
// initial_substate_of
//
/*!
 * \brief Helper for marking initial substate of composite state.
 * \since
 * v.5.5.15
 *
 * Usage example:
 * \code
	class demo : public so_5::agent_t
	{
		state_t active{ *this, "active" };
		state_t wait_input{ initial_substate_of{ active }, "wait_input" };
		state_t dialog{ substate_of{ active }, "dialog" };
		...
	};
 * \endcode
 * 
 * \note A composite state can have only one initial substate.
 */
struct initial_substate_of
{
	state_t * m_parent_state;

	initial_substate_of( state_t & parent_state )
		:	m_parent_state{ &parent_state }
		{}
};

//
// substate_of
//
/*!
 * \brief Helper for marking a substate of composite state.
 * \since
 * v.5.5.15
 *
 * Usage example:
 * \code
	class demo : public so_5::agent_t
	{
		state_t active{ *this, "active" };
		state_t wait_input{ initial_substate_of{ active }, "wait_input" };
		state_t dialog{ substate_of{ active }, "dialog" };
		state_t shutting_down{ substate_of{ active }, "shutdown" };
		...
	};
 * \endcode
 * 
 * \note A composite state can have any number of substates but
 * only one of them must be marked as initial substate.
 */
struct substate_of
{
	state_t * m_parent_state;

	substate_of( state_t & parent_state )
		:	m_parent_state{ &parent_state }
		{}
};

//
// state_t
//

//! Class for the representing agent state.
/*!
 * \attention This class is not thread safe. It is designed to be used inside
 * an owner agent only. For example:
 * \code
	class my_agent : public so_5::agent_t
	{
		state_t first_state{ this, "first" };
		state_t second_state{ this, "second" };
	...
	public :
		my_agent( context_t ctx ) : so_5::agent_t{ ctx }
		{
			// It is a safe usage of state.
			first_state.on_enter( &my_agent::first_on_enter );
			second_state.on_exit( &my_agent::second_on_exit );
			...
		}

		virtual void so_define_agent() override
		{
			// It is a safe usage of state.
			first_state.event( &my_agent::some_event_handler );
			second_state.time_limit( std::chrono::seconds{20}, first_state );

			second_state.event( [this]( const some_message & msg ) {
					// It is also safe usage of state because event handler
					// will be called on the context of agent's working thread.
					second_state.drop_time_limit();
					...
				} );
		}

		void some_public_method()
		{
			// It is a safe usage if this method is called by the agent itself.
			// This will be unsafe usage if this method is called from outside of
			// the agent: a data damage or something like that can happen.
			second_state.time_limit( std::chrono::seconds{30}, first_state );
		}
	...
	};
 * \endcode
 * Because of that be very careful during manipulation of agent's states outside
 * of agent's event handlers.
 */
class SO_5_TYPE state_t final
{
		struct time_limit_t;

		friend class agent_t;

#if defined( SO_5_NEED_GNU_4_8_WORKAROUNDS )
// GCC 4.8.2 reports strange error about usage of deleted copy constructor.
		state_t( const state_t & );
		state_t & operator =( const state_t & );
#else
		state_t( const state_t & ) = delete;
		state_t & operator =( const state_t & ) = delete;
#endif

	public:
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Max deep of nested states.
		 */
		static const std::size_t max_deep = 16;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Type of history for state.
		 */
		enum class history_t
		{
			//! State has no history.
			none,
			//! State has shallow history.
			shallow,
			//! State has deep history.
			deep
		};

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief A type for representation of state's path.
		 *
		 * \note Path contains pointer to the state itself (always the
		 * last item in the path) and pointers to all superstates.
		 * If state has no superstate the path will contains just one
		 * pointer to the state itself.
		 */
		using path_t = std::array< const state_t *, max_deep >;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Type of function to be called on enter to the state.
		 *
		 * \attention Handler must be noexcept function.
		 */
		using on_enter_handler_t = std::function< void() >;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Type of function to be called on exit from the state.
		 *
		 * \attention Handler must be noexcept function.
		 */
		using on_exit_handler_t = std::function< void() >;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Alias for duration type.
		 */
		using duration_t = std::chrono::high_resolution_clock::duration;

		/*!
		 * \note State name will be generated automaticaly.
		 */
		state_t(
			//! State owner.
			agent_t * agent );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \note State name will be generated automaticaly.
		 */
		state_t(
			//! State owner.
			agent_t * agent,
			//! Type of state history.
			history_t state_history );
		state_t(
			//! State owner.
			agent_t * agent,
			//! State name.
			std::string state_name );
		/*!
		 * \since
		 * v.5.5.15
		 */
		state_t(
			//! State owner.
			agent_t * agent,
			//! State name.
			std::string state_name,
			//! Type of state history.
			history_t state_history );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is the initial
		 * substate of some parent state.
		 */
		state_t(
			//! Parent state.
			initial_substate_of parent );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is the initial
		 * substate of some parent state.
		 */
		state_t(
			//! Parent state.
			initial_substate_of parent,
			//! Type of state history.
			history_t state_history );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is the initial
		 * substate of some parent state.
		 */
		state_t(
			//! Parent state.
			initial_substate_of parent,
			//! State name.
			std::string state_name );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is the initial
		 * substate of some parent state.
		 */
		state_t(
			//! Parent state.
			initial_substate_of parent,
			//! State name.
			std::string state_name,
			//! Type of state history.
			history_t state_history );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is a substate of some
		 * parent state.
		 */
		state_t(
			//! Parent state.
			substate_of parent );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is a substate of some
		 * parent state.
		 */
		state_t(
			//! Parent state.
			substate_of parent,
			//! Type of state history.
			history_t state_history );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is a substate of some
		 * parent state.
		 */
		state_t(
			//! Parent state.
			substate_of parent,
			//! State name.
			std::string state_name );
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Constructor for the case when state is a substate of some
		 * parent state.
		 */
		state_t(
			//! Parent state.
			substate_of parent,
			//! State name.
			std::string state_name,
			//! Type of state history.
			history_t state_history );
		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Move constructor.
		 */
		state_t( state_t && other );

		// Explicitely defined destructor is necessary because
		// time_limit_t is incomplete type at this point and
		// compiler can't generate the destructor for unique_ptr<time_limit_t>.
		~state_t();

		bool
		operator==( const state_t & state ) const;

		/*!
		 * \since
		 * v.5.5.20.1
		 */
		bool
		operator!=( const state_t & state ) const
			{
				return !(*this == state);
			}

		//! Get textual name of the state.
		/*!
		 * \note The return type is changed in v.5.5.15: now it is a std::string
		 * object, not a const reference to some value inside state_t object.
		 */
		std::string
		query_name() const;

		//! Does agent owner of this state?
		bool
		is_target( const agent_t * agent ) const;

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Switch agent to that state.
		 */
		void
		activate() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Is this state or any of its substates activated?
		 *
		 * \note See so_5::agent_t::so_is_active_state() for more details.
		 * This method is just a thin wrapper around agent_t::so_is_active_state().
		 */
		bool
		is_active() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Clear state history.
		 *
		 * \note Clears the history for this state only. History for any
		 * substates remains intact. For example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t A{ this, "A", deep_history };
				state_t B{ initial_substate_of{ A }, "B", shallow_history };
				state_t C{ initial_substate_of{ B }, "C" };
				state_t D{ substate_of{ B }, "D" };
				state_t E{ this, "E" };
				...
				void some_event()
				{
					this >>= A; // The current state is "A.B.C"
					            // Because B is initial substate of A, and
					            // C is initial substate of B.
					this >>= D; // The current state is "A.B.D".
					this >>= E; // The current state is "E".
					this >>= A; // The current state is "A.B.D" because deep history of A.
					this >>= E;
					A.clear_history();
					this >>= A; // The current state is "A.B.D" because:
					            // B is the initial substate of A and B has shallow history;
									// D is the last active substate of B.
				}
			};
		 * \endcode
		 *
		 *
		 * \attention This method is not thread safe. Be careful calling
		 * this method from outside of agent's working thread.
		 */
		void
		clear_history()
			{
				m_last_active_substate = nullptr;
			}

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for messages which are
		 * sent to agent's direct mbox.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::agent_t
			{
				const so_5::state_t st_normal = so_make_state();
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
		template< typename... Args >
		const state_t &
		event( Args&&... args ) const;

		/*!
		 * \since
		 * v.5.5.15
		 */
		template< typename... Args >
		state_t &
		event( Args&&... args )
			{
				const state_t & t = *this;
				t.event( std::forward<Args>(args)... );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for messages which are
		 * sent to \a from message-box.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::agent_t
			{
				const so_5::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event( m_owner, [=]( const msg_reconfig & evt ) { ... } );
					st_normal.event( m_owner, &my_agent::evt_shutdown );
					...
				}
			private :
				so_5::mbox_t m_owner;
			};
			\endcode
		 */
		template< typename... Args >
		const state_t &
		event( mbox_t from, Args&&... args ) const;

		/*!
		 * \since
		 * v.5.5.15
		 */
		template< typename... Args >
		state_t &
		event( mbox_t from, Args&&... args )
			{
				const state_t & t = *this;
				t.event( std::move(from), std::forward<Args>(args)... );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for signal subscriptions.
		 * \note This method must be used for messages which are
		 * sent to agent's direct mbox.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::agent_t
			{
				const so_5::state_t st_normal = so_make_state();
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
		template< typename Signal, typename... Args >
		const state_t &
		event( Args&&... args ) const;

		/*!
		 * \since
		 * v.5.5.15
		 */
		template< typename Signal, typename... Args >
		state_t &
		event( Args&&... args )
			{
				const state_t & t = *this;
				t.event< Signal >( std::forward<Args>(args)... );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Helper for subscription of event handler in this state.
		 *
		 * \note This method must be used for signal subscriptions.
		 * \note This method must be used for messages which are
		 * sent to \a from message-box.
		 *
		 * \par Usage example
			\code
			class my_agent : public so_5::agent_t
			{
				const so_5::state_t st_normal = so_make_state();
			public :
				...
				virtual void so_define_agent() override {
					st_normal.event< msg_reconfig >( m_owner, [=] { ... } );
					st_normal.event< msg_shutdown >( m_owner, &my_agent::evt_shutdown );
					...
				}
			private :
				so_5::mbox_t m_owner;
			};
			\endcode
		 */
		template< typename Signal, typename... Args >
		const state_t &
		event( mbox_t from, Args&&... args ) const;

		/*!
		 * \since
		 * v.5.5.15
		 */
		template< typename Signal, typename... Args >
		state_t &
		event( mbox_t from, Args&&... args )
			{
				const state_t & t = *this;
				t.event< Signal >( std::move(from), std::forward<Args>(args)... );
				return *this;
			}

		/*!
		 * \brief Check the presence of a subscription.
		 *
		 * \return true if subscription is present.
		 *
		 * Usage example:
		 * \code
		 * void my_agent::evt_create_new_subscription(mhood_t<data_source> cmd)
		 * {
		 * 	// cmd can contain mbox we have already subscribed to.
		 * 	// If we just call so_subscribe() then an exception can be thrown.
		 * 	// Because of that check the presence of subscription first.
		 * 	if(!some_state.has_subscription<some_message>(cmd->mbox()))
		 * 	{
		 * 		// There is no subscription yet. New subscription can be
		 * 		// created.
		 * 		some_state.event(cmd->mbox(), ...);
		 * 	}
		 * }
		 * \endcode
		 *
		 * \sa so_5::agent_t::so_has_subscription
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< typename Msg >
		bool
		has_subscription( const mbox_t & from ) const;

		/*!
		 * \brief Check the presence of a subscription.
		 *
		 * Type is deduced from the signature of event_handler.
		 *
		 * \return true if subscription is present.
		 *
		 * Usage example:
		 * \code
		 * void my_agent::evt_create_new_subscription(mhood_t<data_source> cmd)
		 * {
		 * 	// cmd can contain mbox we have already subscribed to.
		 * 	// If we just call so_subscribe() then an exception can be thrown.
		 * 	// Because of that check the presence of subscription first.
		 * 	if(!some_state.has_subscription(cmd->mbox(), &my_agent::some_event))
		 * 	{
		 * 		// There is no subscription yet. New subscription can be
		 * 		// created.
		 * 		some_state.event(cmd->mbox(), ...);
		 * 	}
		 * }
		 * \endcode
		 *
		 * \sa so_5::agent_t::so_has_subscription
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< typename Method_Pointer >
		bool
		has_subscription(
			const mbox_t & from,
			Method_Pointer && pfn ) const;

		/*!
		 * \brief Drop subscription.
		 *
		 * Drops the subscription for the message/signal of type \a Msg for
		 * the state (if this subscription is present). Do nothing if subscription
		 * is not exists.
		 *
		 * Usage example:
		 * \code
		 * class my_agent : public so_5::agent_t
		 * {
		 * 	state_t some_state{ this };
		 * 	...
		 * 	virtual void so_define_agent() override
		 * 	{
		 * 		some_state.event(some_mbox, &my_agent::some_handler);
		 * 		...
		 * 	}
		 * 	...
		 * 	void evt_some_another_handler()
		 * 	{
		 * 		// A subscription is no more needed.
		 * 		some_state.drop_subscription<msg>(some_mbox);
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note
		 * This method should be called only from the working context of the agent.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< typename Msg >
		void
		drop_subscription( const mbox_t & from ) const;

		/*!
		 * \brief Drop subscription.
		 *
		 * Drops the subscription for the message/signal for the state (if this
		 * subscription is present). Do nothing if subscription is not exists.
		 *
		 * Type of a message is deduced from event handler signature.
		 *
		 * Usage example:
		 * \code
		 * class my_agent : public so_5::agent_t
		 * {
		 * 	state_t some_state{ this };
		 * 	...
		 * 	virtual void so_define_agent() override
		 * 	{
		 * 		some_state.event(some_mbox, &my_agent::some_handler);
		 * 		...
		 * 	}
		 * 	...
		 * 	void some_another_handler()
		 * 	{
		 * 		// A subscription is no more needed.
		 * 		some_state.drop_subscription(some_mbox, &my_agent::some_handler);
		 * 	}
		 * 	void some_handler(mhood_t<msg> cmd)
		 * 	{
		 * 		...
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note
		 * This method should be called only from the working context of the agent.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< typename Method_Pointer >
		void
		drop_subscription(
			const mbox_t & from,
			Method_Pointer && pfn ) const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief An instruction for switching agent to the specified
		 * state and transfering event proceessing to new state.
		 *
		 * \note Transfers processing of message/signal which is going
		 * from message box \a from.
		 *
		 * \par Usage example:
		 * \code
			class device : public so_5::agent_t {
				const state_t off{ this, "off" };
				const state_t on{ this, "on" };
			public :
				virtual void so_define_agent() override {
					off
						.transfer_to_state< key_on >( some_mbox, on )
						.transfer_to_state< key_info >( some_mbox, on );
					...
				}
				...
			};
		 * \endcode
		 *
		 * \note For more details see subscription_bind_t::transfer_to_state().
		 *
		 * \note Since v.5.5.22.1 actual execution of transfer_to_state operation
		 * can raise so_5::exception_t with so_5::rc_transfer_to_state_loop
		 * error code if a loop in transfer_to_state is detected.
		 */
		template< typename Msg >
		const state_t &
		transfer_to_state( mbox_t from, const state_t & target_state ) const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief An instruction for switching agent to the specified
		 * state and transfering event proceessing to new state.
		 *
		 * \note Transfers processing of message/signal which is going
		 * from agent's direct mbox.
		 *
		 * \par Usage example:
		 * \code
			class device : public so_5::agent_t {
				const state_t off{ this, "off" };
				const state_t on{ this, "on" };
			public :
				virtual void so_define_agent() override {
					off
						.transfer_to_state< key_on >( on )
						.transfer_to_state< key_info >( on );
					...
				}
				...
			};
		 * \endcode
		 *
		 * \note For more details see subscription_bind_t::transfer_to_state().
		 *
		 * \note Since v.5.5.22.1 actual execution of transfer_to_state operation
		 * can raise so_5::exception_t with so_5::rc_transfer_to_state_loop
		 * error code if a loop in transfer_to_state is detected.
		 */
		template< typename Msg >
		const state_t &
		transfer_to_state( const state_t & target_state ) const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief An instruction for switching agent to the specified
		 * state and transfering event proceessing to new state.
		 *
		 * \note Transfers processing of message/signal which is going
		 * from message box \a from.
		 *
		 * \par Usage example:
		 * \code
			class device : public so_5::agent_t {
				state_t off{ this, "off" };
				state_t on{ this, "on" };
			public :
				virtual void so_define_agent() override {
					off
						.transfer_to_state< key_on >( some_mbox, on )
						.transfer_to_state< key_info >( some_mbox, on );
					...
				}
				...
			};
		 * \endcode
		 *
		 * \note For more details see subscription_bind_t::transfer_to_state().
		 *
		 * \note Since v.5.5.22.1 actual execution of transfer_to_state operation
		 * can raise so_5::exception_t with so_5::rc_transfer_to_state_loop
		 * error code if a loop in transfer_to_state is detected.
		 */
		template< typename Msg >
		state_t &
		transfer_to_state( mbox_t from, const state_t & target_state )
			{
				const state_t & t = *this;
				t.transfer_to_state< Msg >( std::move(from), target_state );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief An instruction for switching agent to the specified
		 * state and transfering event proceessing to new state.
		 *
		 * \note Transfers processing of message/signal which is going
		 * from agent's direct mbox.
		 *
		 * \par Usage example:
		 * \code
			class device : public so_5::agent_t {
				state_t off{ this, "off" };
				state_t on{ this, "on" };
			public :
				virtual void so_define_agent() override {
					off
						.transfer_to_state< key_on >( on )
						.transfer_to_state< key_info >( on );
					...
				}
				...
			};
		 * \endcode
		 *
		 * \note For more details see subscription_bind_t::transfer_to_state().
		 *
		 * \note Since v.5.5.22.1 actual execution of transfer_to_state operation
		 * can raise so_5::exception_t with so_5::rc_transfer_to_state_loop
		 * error code if a loop in transfer_to_state is detected.
		 */
		template< typename Msg >
		state_t &
		transfer_to_state( const state_t & target_state )
			{
				const state_t & t = *this;
				t.transfer_to_state< Msg >( target_state );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Define handler which only switches agent to the specified
		 * state.
		 *
		 * \note Defines a reaction to message/signal which is going from
		 * message box \a from.
		 *
		 * \note This method differes from transfer_to_state() method:
		 * just_switch_to() changes state of the agent, but there will not be a
		 * look for event handler for message/signal in the new state. It means
		 * that just_switch_to() is just a shorthand for:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.event< some_signal >( from, [this]{ this >>= S2; } );
			}
		 * \endcode
		 * With just_switch_to() this code can looks like:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.just_switch_to< some_signal >( from, S2 );
			}
		 * \endcode
		 */
		template< typename Msg >
		const state_t &
		just_switch_to( mbox_t from, const state_t & target_state ) const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Define handler which only switches agent to the specified
		 * state.
		 *
		 * \note Defines a reaction to message/signal which is going from
		 * agent's direct mbox.
		 *
		 * \note This method differes from transfer_to_state() method:
		 * just_switch_to() changes state of the agent, but there will not be a
		 * look for event handler for message/signal in the new state. It means
		 * that just_switch_to() is just a shorthand for:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.event< some_signal >( [this]{ this >>= S2; } );
			}
		 * \endcode
		 * With just_switch_to() this code can looks like:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.just_switch_to< some_signal >( S2 );
			}
		 * \endcode
		 */
		template< typename Msg >
		const state_t &
		just_switch_to( const state_t & target_state ) const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Define handler which only switches agent to the specified
		 * state.
		 *
		 * \note Defines a reaction to message/signal which is going from
		 * message box \a from.
		 *
		 * \note This method differes from transfer_to_state() method:
		 * just_switch_to() changes state of the agent, but there will not be a
		 * look for event handler for message/signal in the new state. It means
		 * that just_switch_to() is just a shorthand for:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.event< some_signal >( from, [this]{ this >>= S2; } );
			}
		 * \endcode
		 * With just_switch_to() this code can looks like:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.just_switch_to< some_signal >( from, S2 );
			}
		 * \endcode
		 */
		template< typename Msg >
		state_t &
		just_switch_to( mbox_t from, const state_t & target_state )
			{
				const state_t & t = *this;
				t.just_switch_to< Msg >( std::move(from), target_state );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Define handler which only switches agent to the specified
		 * state.
		 *
		 * \note Defines a reaction to message/signal which is going from
		 * agent's direct mbox.
		 *
		 * \note This method differes from transfer_to_state() method:
		 * just_switch_to() changes state of the agent, but there will not be a
		 * look for event handler for message/signal in the new state. It means
		 * that just_switch_to() is just a shorthand for:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.event< some_signal >( [this]{ this >>= S2; } );
			}
		 * \endcode
		 * With just_switch_to() this code can looks like:
		 * \code
			virtual void demo::so_define_agent() override
			{
				some_state.just_switch_to< some_signal >( S2 );
			}
		 * \endcode
		 */
		template< typename Msg >
		state_t &
		just_switch_to( const state_t & target_state )
			{
				const state_t & t = *this;
				t.just_switch_to< Msg >( target_state );
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Suppress processing of event in this state.
		 *
		 * \note Suppresses message/signal which is going from agent's
		 * direct mbox.
		 *
		 * \note This method allows to disable passing of event to
		 * event handlers from parent states. For example:
		 * \code
			class demo : public so_5::agent_t
			{
				const state_t S1{ this, "1" };
				const state_t S2{ initial_substate_of{ S1 }, "2" };
				const state_t S3{ initial_substate_of{ S2 }, "3" };
			public :
				virtual void so_define_agent() override
				{
					S1
						// Default event handler which will be inherited by states S2 and S3.
						.event< msg1 >(...)
						.event< msg2 >(...)
						.event< msg3 >(...);

					S2
						// A special handler for msg1.
						// For msg2 and msg3 event handlers from state S1 will be used.
						.event< msg1 >(...);

					S3
						// Message msg1 will be suppressed. It will be simply ignored.
						// No events from states S1 and S2 will be called.
						.suppress< msg1 >()
						// The same for msg2.
						.suppress< msg2 >()
						// A special handler for msg3. Overrides handler from state S1.
						.event< msg3 >(...);
				}
			};
		 * \endcode
		 */
		template< typename Msg >
		const state_t &
		suppress() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Suppress processing of event in this state.
		 *
		 * \note Suppresses message/signal which is going from 
		 * message box \a from.
		 *
		 * \note This method allows to disable passing of event to
		 * event handlers from parent states. For example:
		 * \code
			class demo : public so_5::agent_t
			{
				const state_t S1{ this, "1" };
				const state_t S2{ initial_substate_of{ S1 }, "2" };
				const state_t S3{ initial_substate_of{ S2 }, "3" };
			public :
				virtual void so_define_agent() override
				{
					S1
						// Default event handler which will be inherited by states S2 and S3.
						.event< msg1 >( some_mbox, ...)
						.event< msg2 >( some_mbox, ...)
						.event< msg3 >( some_mbox, ...);

					S2
						// A special handler for msg1.
						// For msg2 and msg3 event handlers from state S1 will be used.
						.event< msg1 >( some_mbox, ...);

					S3
						// Message msg1 will be suppressed. It will be simply ignored.
						// No events from states S1 and S2 will be called.
						.suppress< msg1 >( some_mbox )
						// The same for msg2.
						.suppress< msg2 >( some_mbox )
						// A special handler for msg3. Overrides handler from state S1.
						.event< msg3 >( some_mbox );
				}
			};
		 * \endcode
		 */
		template< typename Msg >
		const state_t &
		suppress( mbox_t from ) const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Suppress processing of event in this state.
		 *
		 * \note Suppresses message/signal which is going from agent's
		 * direct mbox.
		 *
		 * \note This method allows to disable passing of event to
		 * event handlers from parent states. For example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t S1{ this, "1" };
				state_t S2{ initial_substate_of{ S1 }, "2" };
				state_t S3{ initial_substate_of{ S2 }, "3" };
			public :
				virtual void so_define_agent() override
				{
					S1
						// Default event handler which will be inherited by states S2 and S3.
						.event< msg1 >(...)
						.event< msg2 >(...)
						.event< msg3 >(...);

					S2
						// A special handler for msg1.
						// For msg2 and msg3 event handlers from state S1 will be used.
						.event< msg1 >(...);

					S3
						// Message msg1 will be suppressed. It will be simply ignored.
						// No events from states S1 and S2 will be called.
						.suppress< msg1 >()
						// The same for msg2.
						.suppress< msg2 >()
						// A special handler for msg3. Overrides handler from state S1.
						.event< msg3 >(...);
				}
			};
		 * \endcode
		 */
		template< typename Msg >
		state_t &
		suppress()
			{
				const state_t & t = *this;
				t.suppress< Msg >();
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Suppress processing of event in this state.
		 *
		 * \note Suppresses message/signal which is going from 
		 * message box \a from.
		 *
		 * \note This method allows to disable passing of event to
		 * event handlers from parent states. For example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t S1{ this, "1" };
				state_t S2{ initial_substate_of{ S1 }, "2" };
				state_t S3{ initial_substate_of{ S2 }, "3" };
			public :
				virtual void so_define_agent() override
				{
					S1
						// Default event handler which will be inherited by states S2 and S3.
						.event< msg1 >( some_mbox, ...)
						.event< msg2 >( some_mbox, ...)
						.event< msg3 >( some_mbox, ...);

					S2
						// A special handler for msg1.
						// For msg2 and msg3 event handlers from state S1 will be used.
						.event< msg1 >( some_mbox, ...);

					S3
						// Message msg1 will be suppressed. It will be simply ignored.
						// No events from states S1 and S2 will be called.
						.suppress< msg1 >( some_mbox )
						// The same for msg2.
						.suppress< msg2 >( some_mbox )
						// A special handler for msg3. Overrides handler from state S1.
						.event< msg3 >( some_mbox );
				}
			};
		 * \endcode
		 */
		template< typename Msg >
		state_t &
		suppress( mbox_t from )
			{
				const state_t & t = *this;
				t.suppress< Msg >( std::move(from) );
				return *this;
			}

		/*!
		 * \name Method for manupulation of enter/exit handlers.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Set on enter handler.
		 *
		 * \attention Handler must be noexcept function. If handler
		 * throws an exception the whole application will be aborted.
		 *
		 * \par Usage example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t number_accumulation{ this, "accumulation" };
				...
			public :
				virtual void so_define_agent() override
				{
					number_accumulation
						.on_enter( [this] { m_number.clear(); } )
						.event( [this]( const next_digit & msg ) {
							m_number += msg.m_digit;
						} );
					...
				}
				...
			private :
				std::string m_number;
			};
		 * \endcode
		 */
		state_t &
		on_enter( on_enter_handler_t handler )
			{
				m_on_enter = std::move(handler);
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Set on enter handler.
		 *
		 * \attention Handler must be noexcept function. If handler
		 * throws an exception the whole application will be aborted.
		 *
		 * \par Usage example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t number_accumulation{ this, "accumulation" };
				...
			public :
				virtual void so_define_agent() override
				{
					number_accumulation
						.on_enter( &demo::accumulation_on_enter )
						.event( [this]( const next_digit & msg ) {
							m_number += msg.m_digit;
						} );
					...
				}
				...
			private :
				std::string m_number;
				...
				void accumulation_on_enter()
				{
					m_number.clear();
					...
				}
			};
		 * \endcode
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				state_t & >::type
		on_enter( Method_Pointer pfn );

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Query on enter handler.
		 *
		 * \note This method can be useful if there is a need to refine
		 * handler defined in the base class:
		 * \code
			class parent : public so_5::agent_t
			{
			protected :
				state_t some_state{ this, ... };

			public :
				virtual void so_define_agent() override
				{
					some_state.on_enter( [this] {
							... // Some action.
						} );
				}
				...
			};
			class child : public parent
			{
			public :
				virtual void so_define_agent() override
				{
					// Calling so_define_agent from base class.
					parent::so_define_agent();

					// Refine on_enter handler for state from parent class.
					auto old_handler = some_state.on_enter();
					some_state.on_enter( [this, old_handler] {
							old_handler(); // Calling the old handler.
							... // Some addition action.
						} );
				}
				...
			};
		 * \endcode
		 */
		const on_enter_handler_t &
		on_enter() const
			{
				return m_on_enter;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Set on exit handler.
		 *
		 * \attention Handler must be noexcept function. If handler
		 * throws an exception the whole application will be aborted.
		 *
		 * \par Usage example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t dialog{ this, "dialog" };
				...
			public :
				virtual void so_define_agent() override
				{
					dialog
						.on_exit( [this] { m_display.turn_off(); } )
						.event( [this]( const user_input & msg ) {...} );
					...
				}
				...
			private :
				display m_display;
			};
		 * \endcode
		 */
		state_t &
		on_exit( on_exit_handler_t handler )
			{
				m_on_exit = std::move(handler);
				return *this;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Set on exit handler.
		 *
		 * \attention Handler must be noexcept function. If handler
		 * throws an exception the whole application will be aborted.
		 *
		 * \par Usage example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t dialog{ this, "dialog" };
				...
			public :
				virtual void so_define_agent() override
				{
					dialog
						.on_exit( &demo::dialog_on_exit )
						.event( [this]( const user_input & msg ) {...} );
					...
				}
				...
			private :
				display m_display;
				...
				void dialog_on_exit()
				{
					m_display.turn_off();
					...
				}
			};
		 * \endcode
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				state_t & >::type
		on_exit( Method_Pointer pfn );

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Query on enter handler.
		 *
		 * \note This method can be useful if there is a need to refine
		 * handler defined in the base class:
		 * \code
			class parent : public so_5::agent_t
			{
			protected :
				state_t some_state{ this, ... };

			public :
				virtual void so_define_agent() override
				{
					some_state.on_exit( [this] {
							... // Some action.
						} );
				}
				...
			};
			class child : public parent
			{
			public :
				virtual void so_define_agent() override
				{
					// Calling so_define_agent from base class.
					parent::so_define_agent();

					// Refine on_enter handler for state from parent class.
					auto old_handler = some_state.on_exit();
					some_state.on_exit( [this, old_handler] {
							... // Some addition action.
							old_handler(); // Calling the old handler.
						} );
				}
				...
			};
		 * \endcode
		 */
		const on_exit_handler_t &
		on_exit() const
			{
				return m_on_exit;
			}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for dealing with state's time limit.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Set up a time limit for the state.
		 *
		 * \note Agent will automatically switched to \a state_to_switch after
		 * \a timeout spent in the current state.
		 *
		 * \par Usage example:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t dialog{ this, "dialog" };
				state_t show_err{ this, "err" };
				...
			public :
				virtual void so_define_agent() override
				{
					dialog
						.on_enter( [this] { m_display.clear(); } )
						.event( [this]( const user_input & msg ) {
								try_handle_input( msg );
								if( some_error() ) {
									m_error = error_description();
									this >>= show_err;
								}
							} )
						...; // Other handlers.
					show_err
						.on_enter( [this] { m_display.show( m_error ); } )
						.time_limit( std::chrono::seconds{2}, dialog );
				}
				...
			private :
				display m_display;
				std::string m_error;
				...
			};
		 * \endcode
		 *
		 * \note If S.time_limit() is called when S is active state then
		 * time_limit for that state will be reset and time for the state S
		 * will be counted from zero.
		 */
		state_t &
		time_limit(
			//! Max duration of time for staying in this state.
			duration_t timeout,
			//! A new state to be switched to.
			const state_t & state_to_switch );

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Drop time limit for the state if defined.
		 *
		 * \note Do nothing if a time limit is not defined.
		 *
		 * \note This method can be useful if there is need to refine state
		 * defined in the parent class:
		 * \code
			class parent : public so_5::agent_t
			{
			protected :
				state_t dialog{ this, "dialog" };
				state_t show_err{ this, "err" };
				...
			public :
				virtual void so_define_agent() override
				{
					dialog
						.on_enter( [this] { m_display.clear(); } )
						.event( [this]( const user_input & msg ) {
								try_handle_input( msg );
								if( some_error() ) {
									m_error = error_description();
									this >>= show_err;
								}
							} )
						...; // Other handlers.
					show_err
						.on_enter( [this] { m_display.show( m_error ); } )
						.time_limit( std::chrono::seconds{2}, dialog );
				}
				...
			protected :
				display m_display;
				std::string m_error;
				...
			};
			class child : public parent
			{
			public :
				virtual void so_define_agent() override
				{
					parent::so_define_agent();

					show_err
						// Switching from show_err to dialog must be done
						// only by pressing "cancel" or "ok" buttons.
						// Automatic switching after 2s must be disabled.
						.drop_time_limit()
						.just_switch_to< key_cancel >( dialog )
						.just_switch_to< key_on >( dialog );
				}
				...
			};
		 * \endcode
		 *
		 * \note This method can be called when agents is already in that
		 * state. Automatic switching will be disabled. To reenable automatic
		 * switching time_limit() method must be called again:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t dialog{ this, "dialog" };
				state_t show_err{ this, "err" };
				...
			public :
				virtual void so_define_agent() override
				{
					dialog
						.on_enter( [this] { m_display.clear(); } )
						.event( [this]( const user_input & msg ) {
								try_handle_input( msg );
								if( some_error() ) {
									m_error = error_description();
									this >>= show_err;
								}
							} )
						...; // Other handlers.
					show_err
						.on_enter( [this] { m_display.show( m_error ); } )
						// By default state will be switched after 2s.
						.time_limit( std::chrono::seconds{2}, dialog )
						.event< key_pause >( [this] {
								// Disable automatic switching.
								show_err.drop_time_limit();
							} )
						.event< key_continue >( [this] {
								// Automatic switching is reenabled.
								show_err.time_limit( std::chrono::seconds{2}, dialog );
								...
							} );
				}
				...
			private :
				display m_display;
				std::string m_error;
				...
			};
		 * \endcode
		 */
		state_t &
		drop_time_limit();
		/*!
		 * \}
		 */

	private:
		//! Fully initialized constructor.
		/*!
		 * \since
		 * v.5.5.15
		 */
		state_t(
			//! Owner of this state.
			agent_t * target_agent,
			//! Name for this state.
			std::string state_name,
			//! Parent state. nullptr means that there is no parent state.
			state_t * parent_state,
			//! Nesting deep for this state. Value 0 means this state is
			//! a top-level state.
			std::size_t nested_level,
			//! Type of state history.
			history_t state_history );

		//! Owner of this state.
		agent_t * const m_target_agent;

		//! State name.
		/*!
		 * \note Since v.5.5.15 has empty value for anonymous state.
		 */
		std::string m_state_name;

		/*!
		 * \since
		 * v.5.5.15
		 * 
		 * \brief Parent state.
		 *
		 * \note Value nullptr means that state is a top-level state and
		 * has no parent state.
		 *
		 * \note This pointer is not const because some modification of
		 * parent state must be performed via that pointer.
		 */
		state_t * m_parent_state;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief The initial substate.
		 *
		 * \note Value nullptr means that state has no initial substate.
		 * If m_substate_count == 0 it is normal. It means that state is
		 * not a composite state. But if m_substate_count != 0 the value
		 * nullptr means that state description is incorrect.
		 */
		const state_t * m_initial_substate;
 
		/*!
		 * \since
		 * v.5.5.15
		 * 
		 * \brief Type of state history.
		 */
		history_t m_state_history;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Last active substate.
		 *
		 * \note This attribute is used only if
		 * m_state_history != history_t::none. It holds a pointer to last
		 * active substate of this composite state.
		 */
		const state_t * m_last_active_substate;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Nesting level for state.
		 *
		 * \note Value 0 means that state is a top-level state.
		 */
		std::size_t m_nested_level;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Number of substates.
		 *
		 * \note Value 0 means that state is not composite state and has no
		 * any substates.
		 */
		size_t m_substate_count;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Handler for the enter to the state.
		 */
		on_enter_handler_t m_on_enter;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Handler for the exit from the state.
		 */
		on_exit_handler_t m_on_exit;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief A definition of time limit for the state.
		 *
		 * \note Value nullptr means that time limit is not set.
		 */
		std::unique_ptr< time_limit_t > m_time_limit;

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief A helper for handle-methods implementation.
		 */
		template< typename... Args >
		const state_t &
		subscribe_message_handler(
			const mbox_t & from,
			Args&&... args ) const;

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief A helper for handle-methods implementation.
		 */
		template< typename Signal, typename... Args >
		const state_t &
		subscribe_signal_handler(
			const mbox_t & from,
			Args&&... args ) const;

		/*!
		 * \name Methods to be used by agents.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Get a parent state if exists.
		 */
		const state_t *
		parent_state() const
			{
				return m_parent_state;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Find actual state to be activated for agent.
		 *
		 * \note If (*this) is a composite state then actual state to
		 * enter will be its m_initial_substate (if m_initial_substate is
		 * a composite state then actual state to enter will be its
		 * m_initial_substate and so on).
		 *
		 * \note If this state is a composite state with history then
		 * m_last_active_substate value will be used (if it is not nullptr).
		 *
		 * \throw exception_t if (*this) is a composite state but m_initial_substate
		 * is not defined.
		 */
		const state_t *
		actual_state_to_enter() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Query nested level for the state.
		 */
		std::size_t
		nested_level() const
			{
				return m_nested_level;
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief A helper method for building a path from top-level
		 * state to this state.
		 */
		void
		fill_path( path_t & path ) const
			{
				path[ m_nested_level ] = this;
				if( m_parent_state )
					m_parent_state->fill_path( path );
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief A helper method which is used during state change for
		 * update state with history.
		 */
		void
		update_history_in_parent_states() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief A special handler of time limit to be used on entering into state.
		 * \attention This method must be called only if m_time_limit is not null.
		 */
		void
		handle_time_limit_on_enter() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief A special handler of time limit to be used on exiting from state.
		 * \attention This method must be called only if m_time_limit is not null.
		 */
		void
		handle_time_limit_on_exit() const;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Call for on enter handler if defined.
		 */
		void
		call_on_enter() const
			{
				if( m_on_enter ) m_on_enter();
				if( m_time_limit ) handle_time_limit_on_enter();
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Call for on exit handler if defined.
		 */
		void
		call_on_exit() const
			{
				if( m_time_limit ) handle_time_limit_on_exit();
				if( m_on_exit ) m_on_exit();
			}
		/*!
		 * \}
		 */
};

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::state_t instead.
 */
using state_t = so_5::state_t;

} /* namespace rt */

} /* namespace so_5 */

