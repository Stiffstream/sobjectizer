/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for agents.
*/

#pragma once

#include <so_5/h/compiler_features.hpp>
#include <so_5/h/declspec.hpp>
#include <so_5/h/types.hpp>
#include <so_5/h/current_thread_id.hpp>
#include <so_5/h/atomic_refcounted.hpp>
#include <so_5/h/spinlocks.hpp>
#include <so_5/h/outliving.hpp>

#include <so_5/h/exception.hpp>
#include <so_5/h/error_logger.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>
#include <so_5/details/h/abort_on_fatal_error.hpp>
#include <so_5/details/h/at_scope_exit.hpp>

#include <so_5/rt/h/fwd.hpp>

#include <so_5/rt/h/agent_ref_fwd.hpp>
#include <so_5/rt/h/agent_context.hpp>
#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/agent_state_listener.hpp>
#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/subscription_storage_fwd.hpp>
#include <so_5/rt/h/handler_makers.hpp>
#include <so_5/rt/h/message_handler_format_detector.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <type_traits>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

/*!
 * \since
 * v.5.3.0
 *
 * \brief A special class which will be used as return value for
 * signal-indication helper.
 *
 * \tparam S type of signal.
 */
template< class S >
struct signal_indicator_t {};

/*!
 * \since
 * v.5.3.0
 *
 * \brief A special signal-indicator.
 *
 * Must be used as signal-indicator in subscription_bind_t::event()
 * methods:
\code
virtual void
my_agent_t::so_define_agent() {
	so_subscribe(mbox).event(
			so_5::signal< get_status >,
			&my_agent_t::evt_get_status );

	so_subscribe(mbox).event(
			so_5::signal< shutdown >,
			[this]() { so_environment().stop(); } );
}
\endcode
 *
 * \tparam S type of signal.
 */
template< class S >
signal_indicator_t< S >
signal() { return signal_indicator_t< S >(); }

//
// exception_reaction_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief A reaction of SObjectizer to an exception from agent event.
 */
enum exception_reaction_t
{
	//! Execution of application must be aborted immediatelly.
	abort_on_exception = 1,
	//! Agent must be switched to special state and SObjectizer
	//! Environment will be stopped.
	shutdown_sobjectizer_on_exception = 2,
	//! Agent must be switched to special state and agent's cooperation
	//! must be deregistered.
	deregister_coop_on_exception = 3,
	//! Exception should be ignored and agent should continue its work.
	ignore_exception = 4,
	/*!
	 * \since
	 * v.5.3.0
	 *
	 * \brief Exception reaction should be inherited from SO Environment.
	 */
	inherit_exception_reaction = 5
};

//
// subscription_bind_t
//

/*!
 * \brief A class for creating a subscription to messages from the mbox.
 *
 * This type provides one of the ways to subscribe an agent's event handlers.
 * There are two way to do that. The first one uses so_5::state_t::event()
 * methods:
 * \code
 * class subscribe_demo : public so_5::agent_t
 * {
 * 	// Some states for the agent.
 * 	state_t st_first{this}, st_second{this}, st_third{this};
 * 	...
 * 	virtual void so_define_agent() override {
 * 		// Subscribe just one event handler for st_first.
 * 		st_first.event(some_mbox, &subscribe_demo::event_handler_1);
 *
 * 		// Subscribe two event handlers for st_second.
 * 		st_second
 * 			.event(some_mbox, &subscribe_demo::event_handler_1)
 * 			.event(some_mbox, &subscribe_demo::event_handler_2);
 *
 * 		// Subscribe two event handlers for st_third.
 * 		st_third
 * 			.event(some_mbox, &subscribe_demo::event_handler_1)
 * 			.event(some_mbox, &subscribe_demo::event_handler_3)
 * 	}
 * };
 * \endcode
 * But this way do not allow to subscribe the same event handler for
 * several states in the compact way.
 *
 * This can be done via agent_t::so_subscribe(), agent_t::so_subscribe_self()
 * and subscription_bind_t object:
 * \code
 * class subscribe_demo : public so_5::agent_t
 * {
 * 	// Some states for the agent.
 * 	state_t st_first{this}, st_second{this}, st_third{this};
 * 	...
 * 	virtual void so_define_agent() override {
 * 		// Subscribe event_handler_1 for all three states
 * 		so_subscribe(some_mbox)
 * 			.in(st_first)
 * 			.in(st_second)
 * 			.in(st_third)
 * 			.event(&subscribe_demo::event_handler_1);
 *
 * 		// Subscribe just one event handler for st_second and st_third.
 * 		so_subscribe(some_mbox)
 * 			.in(st_second)
 * 			.event(&subscribe_demo::event_handler_2);
 *
 * 		// Subscribe two event handlers for st_third.
 * 		so_subscribe(some_mbox)
 * 			.in(st_third)
 * 			.event(&subscribe_demo::event_handler_3)
 * 	}
 * };
 * \endcode
 *
 * \par Some words about binder logic...
 * An object of type subscription_bind_t collects list of states
 * enumerated by calls to subscription_bind_t::in() method.
 * Every call to in() method add a state to that list. It means:
 * \code
 * so_subscribe(some_mbox) // list is: {}
 * 	.in(st_first) // list is: {st_first}
 * 	.in(st_second) // list is: {st_first, st_second}
 * 	.in(st_third) // list is: {st_first, st_second, st_third}
 * 	...
 * \endcode
 * A call to event() or suppress() or just_switch_to() applies subscription
 * to all states which are currently in the list. But these calls do not
 * remove the content of that list. It means:
 * \code
 * so_subscribe(some_mbox) // list is: {}
 * 	.in(st_first) // list is: {st_first}
 * 	.event(handler_1) // subscribe for state st_first only.
 * 	.in(st_second) // list is: {st_first, st_second}
 * 	.event(handler_2) // subscribe for state st_first and for st_second.
 * 	.in(st_third) // list is: {st_first, st_second, st_third}
 * 	.event(handler_3) // subscribe for state st_first, st_second and st_third.
 * 	...
 * \endcode
 */
class subscription_bind_t
{
	public:
		inline
		subscription_bind_t(
			//! Agent to subscribe.
			agent_t & agent,
			//! Mbox for messages to be subscribed.
			const mbox_t & mbox_ref );

		//! Set up a state in which events are allowed be processed.
		inline subscription_bind_t &
		in(
			//! State in which events are allowed.
			const state_t & state );

		//! Make subscription to the message.
		/*!
		 * \since
		 * v.5.5.14
		 *
		 * 
		 * \note Can be used for message and signal handlers.
		 *
		 * \par Usage example
		 * \code
			struct engine_control : public so_5::message_t { ... };
			struct check_status : public so_5::signal_t {};
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
							.event( &engine_controller::control )
							.event( &engine_controller::check_status );
							.event( &engine_controller::accelerate );
					...
				}
				...
			private :
				void control( so_5::mhood_t< engine_control > & cmd ) { ... }
				void check_status( so_5::mhood_t< check_status > & cmd ) { ... }
				void accelerate( so_5::mhood_t< int > & cmd ) { ... }
			};
		 * \endcode
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				subscription_bind_t & >::type
		event(
			//! Event handling method.
			Method_Pointer pfn,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Make subscription to the signal.
		 *
		 * \note This method supports event-methods for signals only.
		 *
		 * \par Usage example
		 * \code
			struct turn_on : public so_5::signal_t {};
			struct turn_off : public so_5::signal_t {};
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
						.event( so_5::signal< turn_on >, &engine_controller::evt_turn_on )
						.event( so_5::signal< turn_off >, &engine_controller::evt_turn_off );
					...
				}
				...
			private :
				void evt_turn_on() { ... }
				void evt_turn_off() { ... }
			};
		 * \endcode
		 *
		 * \attention There is a more convient form of event() for subscription
		 * of signal handlers:
		 * \code
			so_subscribe_self().event< turn_on >( &engine_controller::evt_turn_on );
		 * \endcode
		 *
		 * \deprecated Will be removed in v.5.6.0.
		 */
		template< typename Message, typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				subscription_bind_t & >::type
		event(
			//! Signal indicator.
			signal_indicator_t< Message >(),
			//! Event handling method.
			Method_Pointer pfn,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Make subscription to the message by lambda-function.
		 *
		 * \attention Only lambda-function in the forms:
		 * \code
			Result (const Message &)
			Result (Message)
			Result (so_5::mhood_t<Message>)
			Result (const so_5::mhood_t<Message> &)
		 * \endcode
		 * are supported.
		 *
		 * \par Usage example.
		 * \code
			enum class engine_control { turn_on, turn_off, slow_down };
			struct setup_params : public so_5::message_t { ... };
			struct update_settings { ... };

			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
						.event( [this]( engine_control evt ) {...} )
						.event( [this]( const setup_params & evt ) {...} )
						.event( [this]( const update_settings & evt ) {...} )
					...
				}
				...
			};
		 * \endcode
		 */
		template< class Lambda >
		typename std::enable_if<
				details::lambda_traits::is_lambda<Lambda>::value,
				subscription_bind_t & >::type
		event(
			//! Event handler code.
			Lambda && lambda,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Make subscription to the signal by lambda-function.
		 *
		 * \attention Only lambda-function in the form:
		 * \code
			Result ()
		 * \endcode
		 * is supported.
		 *
		 * \note This method supports event-lambdas for signals only.
		 *
		 * \par Usage example
		 * \code
			struct turn_on : public so_5::signal_t {};
			struct turn_off : public so_5::signal_t {};
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
						.event( so_5::signal< turn_on >, [this]{ ... } )
						.event( so_5::signal< turn_off >, [this]{ ... } );
					...
				}
				...
			};
		 * \endcode
		 *
		 * \attention There is a more convient form of event() for subscription
		 * of signal handlers:
		 * \code
			so_subscribe_self().event< turn_on >( [this]{...} );
		 * \endcode
		 * 
		 * \deprecated Will be removed in v.5.6.0.
		 */
		template< class Message, class Lambda >
		typename std::enable_if<
				details::lambda_traits::is_lambda<Lambda>::value,
				subscription_bind_t & >::type
		event(
			//! Signal indicator.
			signal_indicator_t< Message >(),
			//! Event handling lambda.
			Lambda && lambda,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Make subscription to the signal.
		 *
		 * \par Usage sample:
		 * \code
		   virtual void my_agent::so_define_agent()
		   {
		   	so_subscribe_self().event< msg_my_signal >( &my_agent::event );
		   	so_subscribe_self().event< msg_another_signal >( [=] { ... } );
		   }
		 * \endcode
		 */
		template< typename Signal, typename... Args >
		subscription_bind_t &
		event( Args&&... args )
			{
				return this->event( so_5::signal< Signal >,
						std::forward< Args >(args)... );
			}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief An instruction for switching agent to the specified
		 * state and transfering event proceessing to new state.
		 *
		 * \par Usage example:
		 * \code
			class device : public so_5::agent_t {
				state_t off{ this, "off" };
				state_t on{ this, "on" };
			public :
				virtual void so_define_agent() override {
					so_subscribe_self().in( off )
						.transfer_to_state< key_on >( on )
						.transfer_to_state< key_info >( on );
				}
				...
			};
		 * \endcode
		 *
		 * \note Event will not be postponed or returned to event queue.
		 * A search for a handler for this event will be performed immediately
		 * after switching to the new state.
		 *
		 * \note New state can use transfer_to_state for that event too:
		 * \code
			class device : public so_5::agent_t {
				state_t off{ this, "off" };
				state_t on{ this, "on" };
				state_t status_dialog{ this, "status" };
			public :
				virtual void so_define_agent() override {
					so_subscribe_self().in( off )
						.transfer_to_state< key_on >( on )
						.transfer_to_state< key_info >( on );

					so_subscribe_self().in( on )
						.transfer_to_state< key_info >( status_dialog )
						...;
				}
				...
			};
		 * \endcode
		 *
		 * \note Since v.5.5.22.1 actual execution of transfer_to_state operation
		 * can raise so_5::exception_t with so_5::rc_transfer_to_state_loop
		 * error code if a loop in transfer_to_state is detected.
		 */
		template< typename Msg >
		subscription_bind_t &
		transfer_to_state(
			const state_t & target_state );

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Suppress processing of event in this state.
		 *
		 * \note This method is useful because the event is not passed to
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
					so_subscribe_self().in( S1 )
						// Default event handler which will be inherited by states S2 and S3.
						.event< msg1 >(...)
						.event< msg2 >(...)
						.event< msg3 >(...);

					so_subscribe_self().in( S2 )
						// A special handler for msg1.
						// For msg2 and msg3 event handlers from state S1 will be used.
						.event< msg1 >(...);

					so_subscribe_self().in( S3 )
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
		subscription_bind_t &
		suppress();

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Define handler which only switches agent to the specified
		 * state.
		 *
		 * \note This method differes from transfer_to_state() method:
		 * just_switch_to() changes state of the agent, but there will not be a
		 * look for event handler for message/signal in the new state.  It means
		 * that just_switch_to() is just a shorthand for:
		 * \code
			virtual void demo::so_define_agent() override
			{
				so_subscribe_self().in( S1 )
					.event< some_signal >( [this]{ this >>= S2; } );
			}
		 * \endcode
		 * With just_switch_to() this code can looks like:
		 * \code
			virtual void demo::so_define_agent() override
			{
				so_subscribe_self().in( S1 )
					.just_switch_to< some_signal >( S2 );
			}
		 * \endcode
		 */
		template< typename Msg >
		subscription_bind_t &
		just_switch_to(
			const state_t & target_state );

	private:
		//! Agent to which we are subscribing.
		agent_t * m_agent;
		//! Mbox for messages to subscribe.
		mbox_t m_mbox_ref;

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Type of vector of states.
		 */
		typedef std::vector< const state_t * > state_vector_t;

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief States of agents the event to be subscribed in.
		 */
		state_vector_t m_states;

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Create subscription of event for all states.
		 */
		void
		create_subscription_for_states(
			const std::type_index & msg_type,
			const event_handler_method_t & method,
			thread_safety_t thread_safety ) const;

		/*!
		 * \brief Additional check for subscription to a mutable message
		 * from MPMC mbox.
		 *
		 * Such attempt must be disabled because delivery of mutable
		 * messages via MPMC mboxes is prohibited.
		 *
		 * \throw so_5::exception_t if m_mbox_ref is a MPMC mbox and
		 * \a handler is for mutable message.
		 *
		 * \since
		 * v.5.5.19
		 */
		void
		ensure_handler_can_be_used_with_mbox(
			const so_5::details::msg_type_and_handler_pair_t & handler ) const;
};

//
// agent_t
//

//! A base class for agents.
/*!
	An agent in SObjctizer must be derived from the agent_t.

	The base class provides various methods whose can be splitted into
	the following groups:
	\li methods for the interaction with SObjectizer;
	\li predefined hook-methods which are called during: cooperation
	registration, starting and stopping of an agent;
	\li methods for the message subscription and unsubscription;
	\li methods for working with an agent state;

	<b>Methods for the interaction with SObjectizer</b>

	Method so_5::agent_t::so_environment() serves for the access to the 
	SObjectizer Environment (and, therefore, to all methods of the 
	SObjectizer Environment).
	This method could be called immediatelly after the agent creation.
	This is because agent is bound to the SObjectizer Environment during
	the creation process.

	<b>Hook methods</b>

	The base class defines several hook-methods. Its default implementation
	do nothing.

	The method agent_t::so_define_agent() is called just before agent will
	started by SObjectizer as a part of the agent registration process.
	It should be reimplemented for the initial subscription of the agent
	to messages.

	There are two hook-methods related to important agent's lifetime events:
	agent_t::so_evt_start() and agent_t::so_evt_finish(). They are called
	by SObjectizer in next circumstances:
	- method so_evt_start() is called when the agent is starting its work
	  inside of SObjectizer. At that moment all agents are defined (all 
	  their methods agent_t::so_define_agent() have executed);
	- method so_evt_finish() is called during the agent's cooperation
	  deregistration just after agent processed the last pending event.

	Methods so_evt_start() and so_evt_finish() are called by SObjectizer and
	user can just reimplement them to implement the agent-specific logic.

	<b>Message subscription and unsubscription methods</b>

	Any method with one of the following prototypes can be used as an event
	handler:
	\code
		return_type evt_handler( mhood_t< Message > msg );
		return_type evt_handler( const mhood_t< Message > & msg );
		return_type evt_handler( const Message & msg );
		return_type evt_handler( Message msg );
		return_type evt_handler();
		// Since v.5.5.20:
		return_type evt_handler( mhood_t< Message > msg ) const;
		return_type evt_handler( const mhood_t< Message > & msg ) const;
		return_type evt_handler( const Message & msg ) const;
		return_type evt_handler( Message msg ) const;
		return_type evt_handler() const;
	\endcode
	Where \c evt_handler is a name of the event handler, \c Message is a 
	message type.

	The class mhood_t is a wrapper on pointer to an instance 
	of the \c Message. It is very similar to <tt>std::unique_ptr</tt>. 
	The pointer to \c Message can be a nullptr. It happens in case when 
	the message has no actual data and servers just a signal about something.

	Please note that handlers with the following prototypes can be used
	only for messages, not signals:
	\code
		return_type evt_handler( const Message & msg );
		return_type evt_handler( Message msg );
		// Since v.5.5.20:
		return_type evt_handler( const Message & msg ) const;
		return_type evt_handler( Message msg ) const;
	\endcode

	This form is used only for signals (messages without actual data):
	\code
		return_type evt_handler();
		// Since v.5.5.20:
		return_type evt_handler() const;
	\endcode

	A subscription to the message is performed by the methods so_subscribe()
	and so_subscribe_self().
	This method returns an instance of the so_5::subscription_bind_t which
	does all actual actions of the subscription process. This instance already
	knows agents and message mbox and uses the default agent state for
	the event subscription (binding to different state is also possible). 

	The presence of a subscription can be checked by so_has_subscription()
	method.

	A subscription can be dropped (removed) by so_drop_subscription() and
	so_drop_subscription_for_all_states() methods.

	<b>Deadletter handlers subscription and unsubscription</b>

	Since v.5.5.21 SObjectizer supports deadletter handlers. Such handlers
	are called if there is no any ordinary event handler for a specific
	messages from a specific mbox.

	Deadletter handler can be implemented by an agent method or by lambda
	function. Deadletter handler can have one of the following formats:
	\code
		return_type evt_handler( mhood_t< Message > msg );
		return_type evt_handler( mhood_t< Message > msg ) const;
		return_type evt_handler( const mhood_t< Message > & msg );
		return_type evt_handler( const mhood_t< Message > & msg ) const;
		return_type evt_handler( const Message & msg );
		return_type evt_handler( const Message & msg ) const;
		return_type evt_handler( Message msg );
		return_type evt_handler( Message msg ) const;
	\endcode

	Subscription for a deadletter handler can be created by
	so_subscribe_deadletter_handler() method.

	The presence of a deadletter handler can be checked by
	so_has_deadletter_handler() method.

	A deadletter can be dropped (removed) by so_drop_deadletter_handler()
	and so_drop_subscription_for_all_states() methods.

	<b>Methods for working with an agent state</b>

	The agent can change its state by his so_change_state() method.

	An attempt to switch an agent to the state which belongs to the another
	agent is an error. If state is belong to the same agent there are
	no possibility to any run-time errors. In this case changing agent
	state is a very safe operation.

	In some cases it is necessary to detect agent state switching.
	For example for application monitoring purposes. This can be done
	by "state listeners".

	Any count of state listeners can be set for an agent. There are
	two methods for that:
	- so_add_nondestroyable_listener() is for listeners whose lifetime
	  are controlled by a programmer, not by SObjectizer;
	- so_add_destroyable_listener() is for listeners whose lifetime
	  must be controlled by agent itself.

	<b>Working thread identification</b>

	Since v.5.4.0 some operations for agent are enabled only on agent's
	working thread. They are:
	- subscription management operations (creation or dropping);
	- changing agent's state.

	Working thread for an agent is defined as follows:
	- before invocation of so_define_agent() the working thread is a
	  thread on which agent is created (id of that thread is detected in
	  agent's constructor);
	- during cooperation registration the working thread is a thread on
	  which so_environment::register_coop() is working;
	- after successful agent registration the working thread for it is
	  specified by the dispatcher.

	\note Some dispatchers could provide several working threads for
	an agent. In such case there would not be working thread id. And
	operations like changing agent state or creation of subscription
	would be prohibited after agent registration.
*/
class SO_5_TYPE agent_t
	:	private atomic_refcounted_t
	,	public message_limit::message_limit_methods_mixin_t
{
		friend class subscription_bind_t;
		friend class intrusive_ptr_t< agent_t >;
		friend class coop_t;
		friend class state_t;

		friend class so_5::impl::mpsc_mbox_t;
		friend class so_5::impl::state_switch_guard_t;

		friend class so_5::enveloped_msg::impl::agent_demand_handler_invoker_t;

	public:
		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Short alias for agent_context.
		 */
		using context_t = so_5::agent_context_t;
		/*!
		 * \since
		 * v.5.5.13
		 *
		 * \brief Short alias for %so_5::state_t.
		 */
		using state_t = so_5::state_t;
		/*!
		 * \since
		 * v.5.5.14
		 *
		 * \brief Short alias for %so_5::mhood_t.
		 */
		template< typename T >
		using mhood_t = so_5::mhood_t< T >;
		/*!
		 * \since
		 * v.5.5.19
		 *
		 * \brief Short alias for %so_5::mutable_mhood_t.
		 */
		template< typename T >
		using mutable_mhood_t = so_5::mutable_mhood_t< T >;
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Short alias for %so_5::initial_substate_of.
		 */
		using initial_substate_of = so_5::initial_substate_of;
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Short alias for %so_5::substate_of.
		 */
		using substate_of = so_5::substate_of;
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Short alias for %so_5::state_t::history_t::shallow.
		 */
		static const state_t::history_t shallow_history =
				state_t::history_t::shallow;
		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Short alias for %so_5::state_t::history_t::deep.
		 */
		static const state_t::history_t deep_history =
				state_t::history_t::deep;

		//! Constructor.
		/*!
			Agent must be bound to the SObjectizer Environment during
			its creation. And that binding cannot be changed anymore.
		*/
		explicit agent_t(
			//! The Environment for this agent must exist.
			environment_t & env );

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Constructor which allows specification of
		 * agent's tuning options.
		 *
		 * \par Usage sample:
		 \code
		 using namespace so_5;
		 class my_agent : public agent_t
		 {
		 public :
		 	my_agent( environment_t & env )
				:	agent_t( env, agent_t::tuning_options()
						.subscription_storage_factory(
								vector_based_subscription_storage_factory() ) )
				{...}
		 }
		 \endcode
		 */
		agent_t(
			environment_t & env,
			agent_tuning_options_t tuning_options );

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Constructor which simplifies agent construction with
		 * or without agent's tuning options.
		 *
		 * \par Usage sample:
		 * \code
		 class my_agent : public so_5::agent_t
		 {
		 public :
		 	my_agent( context_t ctx )
				:	so_5::agent( ctx + limit_then_drop< get_status >(1) )
				{}
			...
		 };
		 class my_more_specific_agent : public my_agent
		 {
		 public :
		 	my_more_specific_agent( context_t ctx )
				:	my_agent( ctx + limit_then_drop< reconfigure >(1) )
				{}
		 };

		 // Then somewhere in the code:
		 auto coop = env.create_coop( so_5::autoname );
		 auto a = coop->make_agent< my_agent >();
		 auto b = coop->make_agent< my_more_specific_agent >();
		 * \endcode
		 */
		agent_t( context_t ctx );

		virtual ~agent_t();

		//! Get the raw pointer of itself.
		/*!
			This method is intended for use in the member initialization
			list instead 'this' to suppres compiler warnings.
			For example for an agent state initialization:
			\code
			class a_sample_t : public so_5::agent_t
			{
					typedef so_5::agent_t base_type_t;

					// Agent state.
					const so_5::state_t m_sample_state;
				public:
					a_sample_t( so_5::environment_t & env )
						:
							base_type_t( env ),
							m_sample_state( self_ptr() )
					{
						// ...
					}

				// ...

			};
			\endcode
		*/
		inline const agent_t *
		self_ptr() const
		{
			return this;
		}

		inline agent_t *
		self_ptr()
		{
			return this;
		}

		//! Hook on agent start inside SObjectizer.
		/*!
			It is guaranteed that this method will be called first
			just after end of the cooperation registration process.

			During cooperation registration agent is bound to some
			working thread. And the first method which is called for
			the agent on that working thread context is this method.

			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...
				virtual void
				so_evt_start();
				// ...
			};

			a_sample_t::so_evt_start()
			{
				std::cout << "first agent action on bound dispatcher" << std::endl;
				... // Some application logic actions.
			}
			\endcode
		*/
		virtual void
		so_evt_start();

		//! Hook of agent finish in SObjectizer.
		/*!
			It is guaranteed that this method will be called last
			just before deattaching agent from it's working thread.

			This method should be used to perform some cleanup
			actions on it's working thread.
			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...
				virtual void
				so_evt_finish();
				// ...
			};

			a_sample_t::so_evt_finish()
			{
				std::cout << "last agent activity";

				if( so_current_state() == m_db_error_happened )
				{
					// Delete the DB connection on the same thread where
					// connection was established and where some
					// error happened.
					m_db.release();
				}
			}
			\endcode
		*/
		virtual void
		so_evt_finish();

		//! Access to the current agent state.
		/*!
		 * \note
		 * There is a change in behaviour of this methon in v.5.5.22.
		 * If some on_enter/on_exit handler calls this method during
		 * the state change procedure this method will return the state
		 * for which this on_enter/on_exit handler is called. For example:
		 * \code
		 * class demo final : public so_5::agent_t {
		 * 	state_t st_1{ this };
		 * 	state_t st_1_1{ initial_substate_of{st_1} };
		 * 	state_t st_1_2{ substate_of{st_1}};
		 * 	...
		 * 	virtual void so_define_agent() override {
		 * 		st_1.on_enter([this]{
		 * 			assert(st_1 == so_current_state());
		 * 			...
		 * 		});
		 * 		st_1_1.on_enter([this]{
		 * 			assert(st_1_1 == so_current_state());
		 * 			...
		 * 		});
		 * 		...
		 * 	}
		 * };
		 * \endcode
		 */
		inline const state_t &
		so_current_state() const
		{
			return *m_current_state_ptr;
		}

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Is a state activated?
		 *
		 * \note Since v.5.5.15 a state can have substates. For example
		 * state A can have substates B and C. If B is the current state
		 * then so_current_state() will return a reference to B. But
		 * state A is active too because it is a superstate for B.
		 * Method so_is_active_state(A) will return \a true in that case:
		 * \code
			class demo : public so_5::agent_t
			{
				state_t A{ this, "A" };
				state_t B{ initial_substate_of{ A }, "B" };
				state_t C{ substate_of{ A }, "C" };
				...
				void some_event()
				{
					this >>= C;

					assert( C == so_current_state() );
					assert( !( A == so_current_state() ) );
					assert( so_is_active_state(A) );
					...
				}
			};
		 * \endcode
		 *
		 * \attention This method is not thread safe. Be careful calling
		 * this method from outside of agent's working thread.
		 *
		 * \return \a true if state \a state_to_check is the current state
		 * or if the current state is a substate of \a state_to_check.
		 *
		 */
		bool
		so_is_active_state( const state_t & state_to_check ) const;

		//! Name of the agent's cooperation.
		/*!
		 * \note It is safe to use this method when agent is working inside 
		 * SObjectizer because agent can be registered only in some
		 * cooperation. When agent is not registered in SObjectizer this
		 * method should be used carefully.
		 *
		 * \throw so_5::exception_t If the agent doesn't belong to any cooperation.
		 *
		 * \return Cooperation name if the agent is bound to the cooperation.
		 */
		const std::string &
		so_coop_name() const;

		//! Add a state listener to the agent.
		/*!
		 * A programmer should guarantee that the lifetime of
		 * \a state_listener is exceeds lifetime of the agent.
		 */
		void
		so_add_nondestroyable_listener(
			agent_state_listener_t & state_listener );

		//! Add a state listener to the agent.
		/*!
		 * Agent takes care of the \a state_listener destruction.
		 */
		void
		so_add_destroyable_listener(
			agent_state_listener_unique_ptr_t state_listener );

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief A reaction from SObjectizer to an exception from
		 * agent's event.
		 *
		 * If an exception is going out from agent's event it will be
		 * caught by SObjectizer. Then SObjectizer will call this method
		 * and perform some actions in dependence of return value.
		 *
		 * \note Since v.5.3.0 default implementation calls
		 * coop_t::exception_reaction() for agent's cooperation
		 * object.
		 */
		virtual exception_reaction_t
		so_exception_reaction() const;

		/*!
		 * \since 5.2.3
		 * \brief Switching agent to special state in case of unhandled
		 * exception.
		 */
		void
		so_switch_to_awaiting_deregistration_state();

		//! Push an event to the agent's event queue.
		/*!
			This method is used by SObjectizer for the 
			agent's event scheduling.
		*/
		static inline void
		call_push_event(
			agent_t & agent,
			const message_limit::control_block_t * limit,
			mbox_id_t mbox_id,
			std::type_index msg_type,
			const message_ref_t & message )
		{
			agent.push_event( limit, mbox_id, msg_type, message );
		}

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \brief Push service request to the agent's event queue.
		 *
		 * \deprecated Obsolete in v.5.5.23 and will be removed in v.5.6.0
		 * Use call_push_event() instead. Since v.5.5.23 event type is
		 * automatically detected via message_kind().
		 */
		static inline void
		call_push_service_request(
			agent_t & agent,
			const message_limit::control_block_t * limit,
			mbox_id_t mbox_id,
			std::type_index msg_type,
			const message_ref_t & message )
		{
			agent.push_event( limit, mbox_id, msg_type, message );
		}

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Get the agent's direct mbox.
		 */
		const mbox_t &
		so_direct_mbox() const;

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Create tuning options object with default values.
		 */
		inline static agent_tuning_options_t
		tuning_options()
		{
			return agent_tuning_options_t();
		}

	protected:
		/*!
		 * \name Methods for working with the agent state.
		 * \{
		 */

		//! Access to the agent's default state.
		const state_t &
		so_default_state() const;

	public : /* Note: since v.5.5.1 method so_change_state() is public */

		//! Method changes state.
		/*!
			Usage sample:
			\code
			void a_sample_t::evt_smth( mhood_t< message_one_t > msg )
			{
				// If something wrong with the message then we should
				// switch to the error_state.
				if( error_in_data( *msg ) )
					so_change_state( m_error_state );
			}
			\endcode
		*/
		void
		so_change_state(
			//! New agent state.
			const state_t & new_state );
		/*!
		 * \}
		 */

	public : /* Note: since v.5.2.3.2 subscription-related method are
					made public. */

		/*!
		 * \name Subscription methods.
		 * \{
		 */

		//! Initiate subscription.
		/*!
			This method starts a subscription procedure by returning
			an instance of subscription_bind_t. The subscription details and
			the completion of a subscription is controlled by this
			subscription_bind_t object.

			Usage sample:
			\code
			void a_sample_t::so_define_agent()
			{
				// Subscription for state `state_one`
				so_subscribe( mbox_target )
					.in( state_one )
					.event( &a_sample_t::evt_sample_handler );

				// Subscription for the default state.
				so_subscribe( another_mbox )
					.event( &a_sample_t::evt_another_handler );

				// Subscription for several event handlers in the default state.
				so_subscribe( yet_another_mbox )
					.event( &a_sample_t::evt_yet_another_handler )
					// Lambda-function can be used as event handler too.
					.event( [this](mhood_t<some_message> cmd) {...} );

				// Subscription for several event handlers.
				// All of them will be subscribed for states first_state and second_state.
				so_subscribe( some_mbox )
					.in( first_state )
					.in( second_state )
					.event( &a_sample_t::evt_some_handler_1 )
					.event( &a_sample_t::evt_some_handler_2 )
					.event( &a_sample_t::evt_some_handler_3 );
			}
			\endcode
		*/
		inline subscription_bind_t
		so_subscribe(
			//! Mbox for messages to subscribe.
			const mbox_t & mbox_ref )
		{
			return subscription_bind_t( *this, mbox_ref );
		}

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Initiate subscription to agent's direct mbox.
		 *
		 * Note that is just a short form of:
		 * \code
		 * void a_sample_t::so_define_agent()
		 * {
		 * 	so_subscribe( so_direct_mbox() )
		 * 		.in( some_state )
		 * 		.in( another_state )
		 * 		.event( some_event_handler )
		 * 		.event( some_another_handler );
		 * }
		 * \endcode
		 * Instead of writting `so_subscribe(so_direct_mbox())` it is possible
		 * to write just `so_subscribe_self()`.
		 *
		 * \par Usage sample:
			\code
			void a_sample_t::so_define_agent()
			{
				// Subscription for state `state_one`
				so_subscribe_self()
					.in( state_one )
					.event( &a_sample_t::evt_sample_handler );

				// Subscription for the default state.
				so_subscribe_self()
					.event( &a_sample_t::evt_another_handler );

				// Subscription for several event handlers in the default state.
				so_subscribe_self()
					.event( &a_sample_t::evt_yet_another_handler )
					// Lambda-function can be used as event handler too.
					.event( [this](mhood_t<some_message> cmd) {...} );

				// Subscription for several event handlers.
				// All of them will be subscribed for states first_state and second_state.
				so_subscribe_self()
					.in( first_state )
					.in( second_state )
					.event( &a_sample_t::evt_some_handler_1 )
					.event( &a_sample_t::evt_some_handler_2 )
					.event( &a_sample_t::evt_some_handler_3 );
			}
			\endcode
		 */
		inline subscription_bind_t
		so_subscribe_self()
		{
			return so_subscribe( so_direct_mbox() );
		}

		/*!
		 * \brief Create a subscription for an event.
		 *
		 * \note
		 * Before v.5.5.21 it was a private method. Since v.5.5.21
		 * it is a public method with a standard so_-prefix.
		 * It was made public to allow creation of subscriptions
		 * to agent from outside of agent.
		 */
		void
		so_create_event_subscription(
			//! Message's mbox.
			const mbox_t & mbox_ref,
			//! Message type.
			std::type_index type_index,
			//! State for event.
			const state_t & target_state,
			//! Event handler caller.
			const event_handler_method_t & method,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety );

		/*!
		 * \brief Destroy event subscription.
		 *
		 * \note
		 * This method was introduced in v.5.5.21 to allow manipulation
		 * of agent's subscriptions from outside of an agent.
		 *
		 * \note
		 * It is safe to try to destroy nonexistent subscription.
		 *
		 * \since
		 * v.5.5.21
		 */
		void
		so_destroy_event_subscription(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message's type.
			const std::type_index & subscription_type,
			//! Target state of a subscription.
			const state_t & target_state )
			{
				do_drop_subscription(
						mbox,
						subscription_type,
						target_state );
			}

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Drop subscription for the state specified.
		 *
		 * This overload is indended to be used when there is an event-handler in
		 * the form of agent's method. And there is a need to unsubscribe this
		 * event handler.
		 * For example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void on_some_event(mhood_t<some_msg> cmd) {
		 * 		if(cmd->some_condition)
		 * 			// New subscription must be created.
		 * 			so_subscribe(some_mbox).in(some_state)
		 * 				.event(&demo::one_shot_message_handler);
		 * 		...
		 * 	}
		 *
		 * 	void one_shot_message_handler(mhood_t<another_msg> cmd) {
		 * 		... // Some actions.
		 * 		// Subscription is no more needed.
		 * 		so_drop_subscription(some_mbox, some_state,
		 * 			&demo::one_shot_message_handler);
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				void >::type
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state,
			Method_Pointer /*pfn*/ )
		{
			using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type >::type;

			do_drop_subscription( mbox,
					message_payload_type< message_type >::subscription_type_index(),
					target_state );
		}

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Drop subscription for the state specified.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \deprecated Will be removed in v.5.6.0
		 * Do not use methods which accepts signal_indicator_t.
		 */
		template< class Message >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state,
			signal_indicator_t< Message >() )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< Message >::subscription_type_index(),
					target_state );
		}

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Drop subscription for the state specified.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void on_turn_listening_on(mhood_t<turn_on> cmd) {
		 * 		// New subscription must be created.
		 * 		so_subscribe(cmd->listeting_mbox()).in(some_state)
		 * 			.event([this](mhood_t<state_change_notify> cmd) {...});
		 * 		...
		 * 	}
		 *
		 * 	void on_turn_listening_off(mhood_t<turn_off> cmd) {
		 * 		// Subscription is no more needed.
		 * 		so_drop_subscription<state_change_notify>(cmd->listening_mbox(), some_state);
		 * 		...
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note Doesn't throw if there is no such subscription.
		 */
		template< class Message >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< Message >::subscription_type_index(),
					target_state );
		}

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Drop subscription for the default agent state.
		 *
		 * This overload is indended to be used when there is an event-handler in
		 * the form of agent's method. And there is a need to unsubscribe this
		 * event handler.
		 * For example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void on_some_event(mhood_t<some_msg> cmd) {
		 * 		if(cmd->some_condition)
		 * 			// New subscription must be created.
		 * 			so_subscribe(some_mbox)
		 * 				.event(&demo::one_shot_message_handler);
		 * 		...
		 * 	}
		 *
		 * 	void one_shot_message_handler(mhood_t<another_msg> cmd) {
		 * 		... // Some actions.
		 * 		// Subscription is no more needed.
		 * 		so_drop_subscription(some_mbox,
		 * 			&demo::one_shot_message_handler);
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect Msg type.
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				void >::type
		so_drop_subscription(
			const mbox_t & mbox,
			Method_Pointer /*pfn*/ )
		{
			using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type >::type;

			do_drop_subscription(
					mbox,
					message_payload_type< message_type >::subscription_type_index(),
					so_default_state() );
		}

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Drop subscription for the default agent state.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \deprecated Will be removed in v.5.6.0
		 * Do not use methods which accepts signal_indicator_t.
		 */
		template< class Message >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			signal_indicator_t< Message >() )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< Message >::subscription_type_index(),
					so_default_state() );
		}

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Drop subscription for the default agent state.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void on_turn_listening_on(mhood_t<turn_on> cmd) {
		 * 		// New subscription must be created.
		 * 		so_subscribe(cmd->listening_mbox())
		 * 			.event([this](mhood_t<state_change_notify> cmd) {...});
		 * 		...
		 * 	}
		 *
		 * 	void on_turn_listening_off(mhood_t<turn_off> cmd) {
		 * 		// Subscription is no more needed.
		 * 		so_drop_subscription<state_change_notify>(cmd->listening_mbox());
		 * 		...
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note Doesn't throw if there is no such subscription.
		 */
		template< class Message >
		inline void
		so_drop_subscription(
			const mbox_t & mbox )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< Message >::subscription_type_index(),
					so_default_state() );
		}

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Drop subscription for all states.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	state_t st_working{this}, st_waiting{this}, st_stopping{this};
		 * 	...
		 * 	void on_turn_listening_on(mhood_t<turn_on> cmd) {
		 * 		// Make subscriptions for message of type state_change_notify.
		 * 		st_working.event(cmd->listening_mbox(),
		 * 				&demo::on_state_notify_when_working);
		 * 		st_waiting.event(cmd->listening_mbox(),
		 * 				&demo::on_state_notify_when_waiting);
		 * 		st_waiting.event(cmd->listening_mbox(),
		 * 				&demo::on_state_notify_when_stopping);
		 * 		...
		 * 	}
		 * 	void on_turn_listening_off(mhood_t<turn_off> cmd) {
		 * 		// Subscriptions are no more needed.
		 * 		// All three event handlers for state_change_notify
		 * 		// will be unsubscribed.
		 * 		so_drop_subscription_for_all_states(cmd->listening_mbox(),
		 * 				&demo::on_state_notify_when_working);
		 * 	}
		 * 	...
		 *		void on_state_notify_when_working(mhood_t<state_change_notify> cmd) {...}
		 *		void on_state_notify_when_waiting(mhood_t<state_change_notify> cmd) {...}
		 *		void on_state_notify_when_stopping(mhood_t<state_change_notify> cmd) {...}
		 * };
		 * \endcode
		 *
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect Msg type.
		 *
		 * \note
		 * Since v.5.5.21 this method also drops the subscription
		 * for a deadletter handler for that type of message/signal.
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				void >::type
		so_drop_subscription_for_all_states(
			const mbox_t & mbox,
			Method_Pointer /*pfn*/ )
		{
			using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type >::type;

			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< message_type >::subscription_type_index() );
		}

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Drop subscription for all states.
		 *
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 *
		 * \note
		 * Since v.5.5.21 this method also drops the subscription
		 * for a deadletter handler for that type of signal.
		 *
		 * \deprecated Do not use methods which requires signal_indicator_t.
		 * They will be removed in future versions of SObjectizer.
		 */
		template< class Message >
		inline void
		so_drop_subscription_for_all_states(
			const mbox_t & mbox,
			signal_indicator_t< Message >() )
		{
			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< Message >::subscription_type_index() );
		}

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Drop subscription for all states.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	state_t st_working{this}, st_waiting{this}, st_stopping{this};
		 * 	...
		 * 	void on_turn_listening_on(mhood_t<turn_on> cmd) {
		 * 		// Make subscriptions for message of type state_change_notify.
		 * 		st_working.event(cmd->listening_mbox(),
		 * 				[this](mhood_t<state_change_notify> cmd) {...});
		 * 		st_waiting.event(cmd->listening_mbox(),
		 * 				[this](mhood_t<state_change_notify> cmd) {...});
		 * 		st_waiting.event(cmd->listening_mbox(),
		 * 				[this](mhood_t<state_change_notify> cmd) {...});
		 * 		...
		 * 	}
		 * 	void on_turn_listening_off(mhood_t<turn_off> cmd) {
		 * 		// Subscriptions are no more needed.
		 * 		// All three event handlers for state_change_notify
		 * 		// will be unsubscribed.
		 * 		so_drop_subscription_for_all_states<state_change_notify>(cmd->listening_mbox());
		 * 	}
		 * 	...
		 * };
		 * \endcode
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 *
		 * \note
		 * Since v.5.5.21 this method also drops the subscription
		 * for a deadletter handler for that type of message/signal.
		 */
		template< class Message >
		inline void
		so_drop_subscription_for_all_states(
			const mbox_t & mbox )
		{
			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< Message >::subscription_type_index() );
		}

		/*!
		 * \brief Check the presence of a subscription.
		 *
		 * This method can be used to avoid an exception from so_subscribe()
		 * in the case if the subscription is already present. For example:
		 * \code
		 * void my_agent::evt_create_new_subscription(mhood_t<data_source> cmd)
		 * {
		 * 	// cmd can contain mbox we have already subscribed to.
		 * 	// If we just call so_subscribe() then an exception can be thrown.
		 * 	// Because of that check the presence of subscription first.
		 * 	if(!so_has_subscription<message>(cmd->mbox(), so_default_state()))
		 * 	{
		 * 		// There is no subscription yet. New subscription can be
		 * 		// created.
		 * 		so_subscribe(cmd->mbox()).event(...);
		 * 	}
		 * }
		 * \endcode
		 *
		 * \note
		 * Please do not call this method from outside of working context
		 * of the agent.
		 *
		 * \return true if subscription is present for \a target_state.
		 *
		 * \tparam Message a type of message/signal subscription to which
		 * must be checked.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< class Message >
		bool
		so_has_subscription(
			//! A mbox from which message/signal of type \a Message is expected.
			const mbox_t & mbox,
			//! A target state for the subscription.
			const state_t & target_state ) const SO_5_NOEXCEPT
		{
			return do_check_subscription_presence(
					mbox,
					message_payload_type< Message >::subscription_type_index(),
					target_state );
		}

		/*!
		 * \brief Check the presence of a subscription in the default_state.
		 *
		 * This method can be used to avoid an exception from so_subscribe()
		 * in the case if the subscription is already present. For example:
		 * \code
		 * void my_agent::evt_create_new_subscription(mhood_t<data_source> cmd)
		 * {
		 * 	// cmd can contain mbox we have already subscribed to.
		 * 	// If we just call so_subscribe() then an exception can be thrown.
		 * 	// Because of that check the presence of subscription first.
		 * 	if(!so_has_subscription<message>(cmd->mbox()))
		 * 	{
		 * 		// There is no subscription yet. New subscription can be
		 * 		// created.
		 * 		so_subscribe(cmd->mbox()).event(...);
		 * 	}
		 * }
		 * \endcode
		 *
		 * \note
		 * Please do not call this method from outside of working context
		 * of the agent.
		 *
		 * \return true if subscription is present for the default_state.
		 *
		 * \tparam Message a type of message/signal subscription to which
		 * must be checked.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< class Message >
		bool
		so_has_subscription(
			//! A mbox from which message/signal of type \a Message is expected.
			const mbox_t & mbox ) const SO_5_NOEXCEPT
		{
			return do_check_subscription_presence(
					mbox,
					message_payload_type< Message >::subscription_type_index(),
					so_default_state() );
		}

		/*!
		 * \brief Check the presence of a subscription.
		 *
		 * Type of message is deducted from event-handler signature.
		 *
		 * Usage example:
		 * \code
		 * void my_agent::evt_create_new_subscription(mhood_t<data_source> cmd)
		 * {
		 * 	// cmd can contain mbox we have already subscribed to.
		 * 	// If we just call so_subscribe() then an exception can be thrown.
		 * 	// Because of that check the presence of subscription first.
		 * 	if(!so_has_subscription(cmd->mbox(), my_state, &my_agent::my_event))
		 * 	{
		 * 		// There is no subscription yet. New subscription can be
		 * 		// created.
		 * 		so_subscribe(cmd->mbox()).event(...);
		 * 	}
		 * }
		 * \endcode
		 *
		 * \note
		 * Please do not call this method from outside of working context
		 * of the agent.
		 *
		 * \return true if subscription is present for \a target_state.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				bool >::type
		so_has_subscription(
			//! A mbox from which message/signal is expected.
			const mbox_t & mbox,
			//! A target state for the subscription.
			const state_t & target_state,
			Method_Pointer /*pfn*/ ) const SO_5_NOEXCEPT
		{
			using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type>::type;

			return this->so_has_subscription<message_type>(
					mbox, target_state );
		}

		/*!
		 * \brief Check the presence of a subscription.
		 *
		 * Subscription is checked for the default agent state.
		 *
		 * Type of message is deducted from event-handler signature.
		 *
		 * Usage example:
		 * \code
		 * void my_agent::evt_create_new_subscription(mhood_t<data_source> cmd)
		 * {
		 * 	// cmd can contain mbox we have already subscribed to.
		 * 	// If we just call so_subscribe() then an exception can be thrown.
		 * 	// Because of that check the presence of subscription first.
		 * 	if(!so_has_subscription(cmd->mbox(), &my_agent::my_event))
		 * 	{
		 * 		// There is no subscription yet. New subscription can be
		 * 		// created.
		 * 		so_subscribe(cmd->mbox()).event(...);
		 * 	}
		 * }
		 * \endcode
		 *
		 * \note
		 * Please do not call this method from outside of working context
		 * of the agent.
		 *
		 * \return true if subscription is present for the default state.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<Method_Pointer>::value,
				bool >::type
		so_has_subscription(
			//! A mbox from which message/signal is expected.
			const mbox_t & mbox,
			Method_Pointer /*pfn*/ ) const SO_5_NOEXCEPT
		{
			using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type>::type;

			return this->so_has_subscription<message_type>(
					mbox, so_default_state() );
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for dealing with deadletter subscriptions.
		 * \{
		 */
		/*!
		 * \brief Create a subscription for a deadletter handler.
		 *
		 * \note
		 * This is low-level method intended to be used by libraries writters.
		 * Do not call it directly if you don't understand its purpose and
		 * what its arguments mean. Use so_subscribe_deadletter_handler()
		 * instead.
		 *
		 * This method actually creates a subscription to deadletter handler
		 * for messages/signal of type \a msg_type from mbox \a mbox.
		 *
		 * \throw so_5::exception_t in the case when the subscription 
		 * of a deadletter handler for type \a msg_type from \a mbox is
		 * already exists.
		 *
		 * \since
		 * v.5.5.21
		 */
		void
		so_create_deadletter_subscription(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type,
			//! Event handler caller.
			const event_handler_method_t & method,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety );

		/*!
		 * \brief Destroy a subscription for a deadletter handler.
		 *
		 * \note
		 * This is low-level method intended to be used by libraries writters.
		 * Do not call it directly if you don't understand its purpose and
		 * what its arguments mean. Use so_drop_deadletter_handler() instead.
		 *
		 * This method actually destroys a subscription to deadletter handler
		 * for messages/signal of type \a msg_type from mbox \a mbox.
		 *
		 * \note
		 * It is safe to call this method if there is no such
		 * deadletter handler. It will do nothing in such case.
		 *
		 * \since
		 * v.5.5.21
		 */
		void
		so_destroy_deadletter_subscription(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type );

		/*!
		 * \brief Create a subscription for deadletter handler for
		 * a specific message from a specific mbox.
		 *
		 * Type of a message for deadletter handler will be detected
		 * automatically from the signature of the \a handler.
		 *
		 * A deadletter handler can be a pointer to method of agent or
		 * lambda-function. The handler should have one of the following
		 * format:
		 * \code
		 * ret_type deadletter_handler(message_type);
		 * ret_type deadletter_handler(const message_type &);
		 * ret_type deadletter_handler(mhood_t<message_type>);
		 * \endcode
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void on_some_message(mhood_t<some_message> cmd) {...}
		 * 	...
		 * 	virtual void so_define_agent() override {
		 * 		// Create deadletter handler via pointer to method.
		 * 		// Event handler will be not-thread-safe.
		 * 		so_subscribe_deadletter_handler(
		 * 			so_direct_mbox(),
		 * 			&demo::on_some_message );
		 *
		 * 		// Create deadletter handler via lambda-function.
		 * 		so_subscribe_deadletter_handler(
		 * 			status_mbox(), // Any mbox can be used, not only agent's direct mbox.
		 * 			[](mhood_t<status_request>) -> std::string {
		 * 				return "working";
		 * 			},
		 *				// This handler will be thread-safe one.
		 *				so_5::thread_safe );
		 * 	}
		 * };
		 * \endcode
		 *
		 * \throw so_5::exception_t in the case when the subscription 
		 * of a deadletter handler for type \a msg_type from \a mbox is
		 * already exists.
		 *
		 * \since
		 * v.5.5.21
		 */
		template< typename Event_Handler >
		void
		so_subscribe_deadletter_handler(
			const so_5::mbox_t & mbox,
			Event_Handler && handler,
			thread_safety_t thread_safety = thread_safety_t::unsafe )
			{
				using namespace details::event_subscription_helpers;

				const auto ev = preprocess_agent_event_handler(
						mbox,
						*this,
						std::forward<Event_Handler>(handler) );

				so_create_deadletter_subscription(
						mbox,
						ev.m_msg_type,
						ev.m_handler,
						thread_safety );
			}

		/*!
		 * \brief Drops the subscription for deadletter handler.
		 *
		 * A message type must be specified explicitely via template
		 * parameter.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void some_deadletter_handler(mhood_t<some_message> cmd) {
		 * 		... // Do some stuff.
		 * 		// There is no need for deadletter handler.
		 * 		so_drop_deadletter_handler<some_message>(some_mbox);
		 * 	}
		 * 	...
		 * };
		 * \endcode
		 *
		 * \note
		 * Is is safe to call this method if there is no a deadletter
		 * handler for message of type \a Message from message box
		 * \a mbox.
		 *
		 * \tparam Message Type of a message or signal for deadletter
		 * handler.
		 *
		 * \since
		 * v.5.5.21
		 */
		template< typename Message >
		void
		so_drop_deadletter_handler(
			//! A mbox from which the message is expected.
			const so_5::mbox_t & mbox )
			{
				so_destroy_deadletter_subscription(
						mbox,
						message_payload_type< Message >::subscription_type_index() );
			}

		/*!
		 * \brief Checks the presence of deadletter handler for a message of
		 * a specific type from a specific mbox.
		 *
		 * Message type must be specified explicitely via template
		 * parameter \a Message.
		 *
		 * \return true if a deadletter for a message/signal of type
		 * \a Message from message mbox \a mbox exists.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * 	void on_some_request(mhood_t<request_data> cmd) {
		 * 		if(!so_has_deadletter_handler<some_message>(some_mbox))
		 * 			// There is no deadletter handler yet.
		 * 			// It should be created now.
		 * 			so_subscribe_deadletter_handler(
		 * 				some_mbox,
		 * 				[this](mhood_t<some_message> cmd) {...});
		 * 		...
		 * 	}
		 * };
		 * \endcode
		 *
		 * \tparam Message Type of a message or signal for deadletter
		 * handler.
		 *
		 * \since
		 * v.5.5.21
		 */
		template< typename Message >
		bool
		so_has_deadletter_handler(
			//! A mbox from which the message is expected.
			const so_5::mbox_t & mbox ) const SO_5_NOEXCEPT
			{
				return do_check_deadletter_presence(
						mbox,
						message_payload_type< Message >::subscription_type_index() );
			}
		/*!
		 * \}
		 */

	protected :

		/*!
		 * \name Agent initialization methods.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief A correct initiation of so_define_agent method call.
		 *
		 * Before the actual so_define_agent() method it is necessary
		 * to temporary set working thread id. And then drop this id
		 * to non-actual value after so_define_agent() return.
		 *
		 * Because of that this method must be called during cooperation
		 * registration procedure instead of direct call of so_define_agent().
		 */
		void
		so_initiate_agent_definition();

		//! Hook on define agent for SObjectizer.
		/*!
			This method is called by SObjectizer during the cooperation
			registration process before agent will be bound to its
			working thread.

			Should be used by the agent to make necessary message subscriptions.

			Usage sample;
			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...
				virtual void
				so_define_agent();

				void evt_handler_1( mhood_t< message1 > msg );
				// ...
				void evt_handler_N( mhood_t< messageN > & msg );

			};

			void
			a_sample_t::so_define_agent()
			{
				// Make subscriptions...
				so_subscribe( m_mbox1 )
					.in( m_state_1 )
						.event( &a_sample_t::evt_handler_1 );
				// ...
				so_subscribe( m_mboxN )
					.in( m_state_N )
						.event( &a_sample_t::evt_handler_N );
			}
			\endcode
		*/
		virtual void
		so_define_agent();

		//! Is method define_agent already called?
		/*!
			Usage sample:
			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...

				public:
					void
					set_target_mbox( const so_5::mbox_t & mbox )
					{
						// mbox cannot be changed after agent registration.
						if( !so_was_defined() )
						{
							m_target_mbox = mbox;
						}
					}

				private:
					so_5::mbox_t m_target_mbox;
			};
			\endcode
		*/
		bool
		so_was_defined() const;
		/*!
		 * \}
		 */

	public:
		//! Access to the SObjectizer Environment which this agent is belong.
		/*!
			Usage sample for other cooperation registration:
			\code
			void a_sample_t::evt_on_smth( mhood_t< some_message_t > msg )
			{
				so_5::coop_unique_ptr_t coop =
					so_environment().create_coop( "first_coop" );

				// Filling the cooperation...
				coop->make_agent< a_another_t >( ... );
				// ...

				// Registering cooperation.
				so_environment().register_coop( std::move(coop) );
			}
			\endcode

			Usage sample for the SObjectizer shutting down:
			\code
			void a_sample_t::evt_last_event( mhood_t< message_one_t > msg )
			{
				...
				so_environment().stop();
			}
			\endcode
		*/
		environment_t &
		so_environment() const;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Binding agent to the dispatcher.
		 *
		 * This is an actual start of agent's work in SObjectizer.
		 */
		void
		so_bind_to_dispatcher(
			//! Actual event queue for an agent.
			event_queue_t & queue );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Create execution hint for the specified demand.
		 *
		 * The hint returned is intendent for the immediately usage.
		 * It must not be stored for the long time and used sometime in
		 * the future. It is because internal state of the agent
		 * can be changed and some references from hint object to
		 * agent's internals become invalid.
		 */
		static execution_hint_t
		so_create_execution_hint(
			//! Demand for execution of event handler.
			execution_demand_t & demand );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief A helper method for deregistering agent's coop.
		 */
		void
		so_deregister_agent_coop( int dereg_reason );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief A helper method for deregistering agent's coop
		 * in case of normal deregistration.
		 *
		 * \note It is just a shorthand for:
			\code
			so_deregister_agent_coop( so_5::dereg_reason::normal );
			\endcode
		 */
		void
		so_deregister_agent_coop_normally();

		/*!
		 * \name Methods for dealing with message delivery filters.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Set a delivery filter.
		 *
		 * \tparam Message type of message to be filtered.
		 */
		template< typename Message >
		void
		so_set_delivery_filter(
			//! Message box from which message is expected.
			//! This must be MPMC-mbox.
			const mbox_t & mbox,
			//! Delivery filter instance.
			delivery_filter_unique_ptr_t filter )
			{
				ensure_not_signal< Message >();

				do_set_delivery_filter(
						mbox,
						message_payload_type< Message >::subscription_type_index(),
						std::move(filter) );
			}

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Set a delivery filter.
		 *
		 * \tparam Lambda type of lambda-function or functional object which
		 * must be used as message filter.
		 *
		 * \par Usage sample:
		 \code
		 void my_agent::so_define_agent() {
		 	so_set_delivery_filter( temp_sensor,
				[]( const current_temperature & msg ) {
					return !is_normal_temperature( msg );
				} );
			...
		 }
		 \endcode
		 */
		template< typename Lambda >
		void
		so_set_delivery_filter(
			//! Message box from which message is expected.
			//! This must be MPMC-mbox.
			const mbox_t & mbox,
			//! Delivery filter as lambda-function or functional object.
			Lambda && lambda );

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Drop a delivery filter.
		 *
		 * \tparam Message type of message filtered.
		 */
		template< typename Message >
		void
		so_drop_delivery_filter(
			//! Message box to which delivery filter was set.
			//! This must be MPMC-mbox.
			const mbox_t & mbox ) SO_5_NOEXCEPT
			{
				do_drop_delivery_filter(
						mbox,
						message_payload_type< Message >::subscription_type_index() );
			}
		/*!
		 * \}
		 */

		/*!
		 * \name Dealing with priority.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Get the priority of the agent.
		 */
		priority_t
		so_priority() const
			{
				return m_priority;
			}
		/*!
		 * \}
		 */

	protected :
		/*!
		 * \name Helpers for state object creation.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Helper method for creation of anonymous state object.
		 *
		 * \par Usage:
		 	\code
			class my_agent_t : public so_5::agent_t
			{
				so_5::state_t st_1 = so_make_state();
				so_5::state_t st_2 = so_make_state();
				...
			};
			\endcode
		 *
		 * \deprecated Will be removed in v.5.6.0.
		 * Just use ordinary constructors of state_t:
		 	\code
			class my_agent_t : public so_5::agent_t
			{
				state_t st_1{ this };
				state_t st_2{ this };
				...
			}
			\endcode
		 */
		inline state_t
		so_make_state()
		{
			return state_t( self_ptr() );
		}

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Helper method for creation of named state object.
		 *
		 * \par Usage:
		 	\code
			class my_agent_t : public so_5::agent_t
			{
				so_5::state_t st_1 = so_make_state( "st_one" );
				so_5::state_t st_2 = so_make_state( "st_two" );
				...
			};
			\endcode
		 *
		 *
		 * \deprecated Will be removed in v.5.6.0.
		 * Just use ordinary constructors of state_t:
		 	\code
			class my_agent_t : public so_5::agent_t
			{
				state_t st_1{ this, "st_one" };
				state_t st_2{ this, "st_two" };
				...
			}
			\endcode
		 */
		inline state_t
		so_make_state( std::string name )
		{
			return state_t( self_ptr(), std::move( name ) );
		}
		/*!
		 * \}
		 */

	private:
		const state_t st_default = so_make_state( "<DEFAULT>" );

		//! Current agent state.
		const state_t * m_current_state_ptr;

		/*!
		 * \brief Enumeration of possible agent statuses.
		 *
		 * \since
		 * v.5.5.18
		 */
		enum class agent_status_t : char
		{
			//! Agent is not defined yet.
			//! This is an initial agent status.
			not_defined_yet,
			//! Agent is defined.
			defined,
			//! State switch operation is in progress.
			state_switch_in_progress
		};

		/*!
		 * \brief Current agent status.
		 *
		 * \since
		 * v.5.5.18
		 */
		agent_status_t m_current_status;

		//! State listeners controller.
		std::unique_ptr< impl::state_listener_controller_t >
			m_state_listener_controller;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Type of function for searching event handler.
		 */
		using handler_finder_t = 
			const impl::event_handler_data_t *(*)(
					execution_demand_t & /* demand */,
					const char * /* context_marker */ );

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Function for searching event handler.
		 *
		 * \note The value is set only once in the constructor and
		 * doesn't changed anymore.
		 */
		handler_finder_t m_handler_finder;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief All agent's subscriptions.
		 */
		impl::subscription_storage_unique_ptr_t m_subscriptions;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Run-time information for message limits.
		 *
		 * Created only of message limits are described in agent's
		 * tuning options.
		 *
		 * \attention This attribute must be initialized before the
		 * \a m_direct_mbox attribute. It is because the value of
		 * \a m_message_limits is used in \a m_direct_mbox creation.
		 * Because of that \a m_message_limits is declared before
		 * \a m_direct_mbox.
		 */
		std::unique_ptr< message_limit::impl::info_storage_t > m_message_limits;

		//! SObjectizer Environment for which the agent is belong.
		environment_t & m_env;

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Event queue operation protector.
		 *
		 * Initially m_event_queue is NULL. It is changed to actual value
		 * in so_bind_to_dispatcher() method. And reset to nullptr again
		 * in shutdown_agent().
		 *
		 * nullptr in m_event_queue means that methods push_event() and
		 * push_service_request() will throw away any new demand.
		 *
		 * It is necessary to provide guarantee that m_event_queue will
		 * be reset to nullptr in shutdown_agent() only if there is no
		 * working push_event()/push_service_request() methods. To do than
		 * default_rw_spinlock_t is used. Methods push_event() and
		 * push_service_request() acquire it in read-mode and shutdown_agent()
		 * acquires it in write-mode. It means that shutdown_agent() cannot
		 * get access to m_event_queue until there is working
		 * push_event()/push_service_request().
		 */
		default_rw_spinlock_t	m_event_queue_lock;

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief A pointer to event_queue.
		 *
		 * After binding to the dispatcher is it pointed to the actual
		 * event queue.
		 *
		 * After shutdown it is set to nullptr.
		 *
		 * \attention Access to m_event_queue value must be done only
		 * under acquired m_event_queue_lock.
		 */
		event_queue_t * m_event_queue;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief A direct mbox for the agent.
		 */
		const mbox_t m_direct_mbox;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Working thread id.
		 *
		 * Some actions like managing subscriptions and changing states
		 * are enabled only on working thread id.
		 */
		so_5::current_thread_id_t m_working_thread_id;

		//! Agent is belong to this cooperation.
		coop_t * m_agent_coop;

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Delivery filters for that agents.
		 *
		 * \note Storage is created only when necessary.
		 */
		std::unique_ptr< impl::delivery_filter_storage_t > m_delivery_filters;

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Priority of the agent.
		 */
		const priority_t m_priority;

		//! Make an agent reference.
		/*!
		 * This is an internal SObjectizer method. It is called when
		 * it is guaranteed that the agent is still necessary and something
		 * has reference to it.
		 */
		agent_ref_t
		create_ref();

		/*!
		 * \name Embedding agent into the SObjectizer Run-time.
		 * \{
		 */

		//! Bind agent to the cooperation.
		/*!
		 * Initializes an internal cooperation pointer.
		 *
		 * Drops m_is_coop_deregistered to false.
		 */
		void
		bind_to_coop(
			//! Cooperation for that agent.
			coop_t & coop );

		//! Agent shutdown deriver.
		/*!
		 * \since
		 * v.5.2.3
		 *
		 *
		 * Method destroys all agent subscriptions.
		 */
		void
		shutdown_agent() SO_5_NOEXCEPT;
		/*!
		 * \}
		 */

		/*!
		 * \name Subscription/unsubscription implementation details.
		 * \{
		 */

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Detect limit for that message type.
		 *
		 * \return nullptr if message limits are not used.
		 *
		 * \throw exception_t if message limits are used but the limit
		 * for that message type is not found.
		 */
		const message_limit::control_block_t *
		detect_limit_for_message_type(
			const std::type_index & msg_type ) const;

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Remove subscription for the state specified.
		 */
		void
		do_drop_subscription(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type,
			//! State for event.
			const state_t & target_state );

		/*!
		 * \since
		 * v.5.2.3
		 *
		 * \brief Remove subscription for all states.
		 */
		void
		do_drop_subscription_for_all_states(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type );

		/*!
		 * \brief Check the presence of a subscription.
		 *
		 * \since
		 * v.5.5.19.5
		 */
		bool
		do_check_subscription_presence(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type,
			//! State for the subscription.
			const state_t & target_state ) const SO_5_NOEXCEPT;

		/*!
		 * \brief Check the presence of a deadletter handler.
		 *
		 * \since
		 * v.5.5.21
		 */
		bool
		do_check_deadletter_presence(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type ) const SO_5_NOEXCEPT;
		/*!
		 * \}
		 */


		/*!
		 * \name Event handling implementation details.
		 * \{
		 */

		//! Push event into the event queue.
		void
		push_event(
			//! Optional message limit.
			const message_limit::control_block_t * limit,
			//! ID of mbox for this event.
			mbox_id_t mbox_id,
			//! Message type for event.
			std::type_index msg_type,
			//! Event message.
			const message_ref_t & message );
		/*!
		 * \}
		 */

		// NOTE: demand handlers declared as public to allow
		// access this handlers from unit-tests.
	public :
		/*!
		 * \name Demand handlers.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Calls so_evt_start method for agent.
		 */
		static void
		demand_handler_on_start(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Ensures that all agents from cooperation are
		 * bound to dispatchers.
		 */
		void
		ensure_binding_finished();

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_start_ptr() SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Calls so_evt_finish method for agent.
		 */
		static void
		demand_handler_on_finish(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_finish_ptr() SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Calls event handler for message.
		 */
		static void
		demand_handler_on_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_message_ptr() SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.3.0
		 *
		 * \note
		 * Name of that method changed in v.5.5.23.
		 *
		 * \brief Calls request handler for message.
		 */
		static void
		demand_handler_on_service_request(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_service_request_handler_on_message_ptr() SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.5.23
		 *
		 * \brief Handles the enveloped message.
		 */
		static void
		demand_handler_on_enveloped_msg(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since
		 * v.5.5.24
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_enveloped_msg_ptr() SO_5_NOEXCEPT;
		/*!
		 * \}
		 */

	private :
		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Actual implementation of message handling.
		 *
		 * \note Since v.5.5.17.1 argument \a method is passed as copy.
		 * It prevents deallocation of event_handler_method in the following
		 * case:
		 * \code
		 	auto mbox = so_environment().create_mbox();
			so_subscribe( mbox ).event< some_signal >( [this, mbox] {
				so_drop_subscription< some_signal >( mbox );
				... // Some other actions.
			} );
		 * \endcode
		 */
		static void
		process_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			event_handler_method_t method );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Actual implementation of service request handling.
		 *
		 * \note handler_data.first == true only if handler_data.second is an
		 * actual result of searching handler for the message. If
		 * handler_data.first == second then it is necessary to search event
		 * handler for the message.
		 */
		static void
		process_service_request(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			std::pair< bool, const impl::event_handler_data_t * > handler_data );

		/*!
		 * \brief Actual implementation of enveloped message handling.
		 *
		 * \note
		 * handler_data can be nullptr. It means that an event handler
		 * for that message type if not found and special hook will
		 * be called for the envelope.
		 *
		 * \since
		 * v.5.5.23
		 */
		static void
		process_enveloped_msg(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			const impl::event_handler_data_t * handler_data );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Enables operation only if it is performed on agent's
		 * working thread.
		 */
		void
		ensure_operation_is_on_working_thread(
			const char * operation_name ) const;

		/*!
		 * \since
		 * v.5.5.0
		 *
		 * \brief Drops all delivery filters.
		 */
		void
		drop_all_delivery_filters() SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Set a delivery filter.
		 */
		void
		do_set_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			delivery_filter_unique_ptr_t filter );

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Drop a delivery filter.
		 */
		void
		do_drop_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type ) SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Handler finder for the case when message delivery
		 * tracing is disabled.
		 */
		static const impl::event_handler_data_t *
		handler_finder_msg_tracing_disabled(
			execution_demand_t & demand,
			const char * context_marker );

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Handler finder for the case when message delivery
		 * tracing is enabled.
		 */
		static const impl::event_handler_data_t *
		handler_finder_msg_tracing_enabled(
			execution_demand_t & demand,
			const char * context_marker );

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Actual search for event handler with respect
		 * to parent-child relationship between agent states.
		 */
		static const impl::event_handler_data_t *
		find_event_handler_for_current_state(
			execution_demand_t & demand );

		/*!
		 * \brief Search for event handler between deadletter handlers.
		 *
		 * \return nullptr if event handler is not found.
		 *
		 * \since
		 * v.5.5.21
		 */
		static const impl::event_handler_data_t *
		find_deadletter_handler(
			execution_demand_t & demand );

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Actual action for switching agent state.
		 */
		void
		do_state_switch(
			//! New state to be set as the current state.
			const state_t & state_to_be_set ) SO_5_NOEXCEPT;

		/*!
		 * \since
		 * v.5.5.15
		 *
		 * \brief Return agent to the default state.
		 *
		 * \note This method is called just before invocation of
		 * so_evt_finish() to return agent to the default state.
		 * This return will initiate invocation of on_exit handlers
		 * for all active states of the agent.
		 *
		 * \attention State switch is not performed is agent is already
		 * in default state or if it waits deregistration after unhandled
		 * exception.
		 */
		void
		return_to_default_state_if_possible() SO_5_NOEXCEPT;
};

/*!
 * \since
 * v.5.5.5
 *
 * \brief Template-based implementations of delivery filters.
 */
namespace delivery_filter_templates
{

/*!
 * \since
 * v.5.5.5
 *
 * \brief An implementation of delivery filter represented by lambda-function
 * like object.
 *
 * \tparam Lambda type of lambda-function or functional object.
 */
template< typename Lambda, typename Message >
class lambda_as_filter_t : public delivery_filter_t
	{
		Lambda m_filter;

		// NOTE: this is a workaround for strage behaviour of VC++ 19.0.23918
		// (from Visual Studio 2015 Update 2).
		// It seems that noexcept on check() doesn't work.
		// Because of that it is necessary to call another noexcept
		// function from check().
		bool
		do_check( Message & m ) const SO_5_NOEXCEPT
		{
			return m_filter( m );
		}

	public :
		lambda_as_filter_t( Lambda && filter )
			:	m_filter( std::forward< Lambda >( filter ) )
			{}

#if SO_5_HAVE_NOEXCEPT
		virtual bool
		check(
			const agent_t & /*receiver*/,
			message_t & msg ) const SO_5_NOEXCEPT override
			{
				return do_check(message_payload_type< Message >::payload_reference( msg ));
			}
#else
		virtual bool
		check(
			const agent_t & receiver,
			message_t & msg ) const SO_5_NOEXCEPT override
			{
				return so_5::details::do_with_rollback_on_exception(
					[&] {
						return m_filter( message_payload_type< Message >::payload_reference( msg ) );
					},
					[&] {
						so_5::details::abort_on_fatal_error( [&] {
							SO_5_LOG_ERROR( receiver.so_environment(), serr ) {
								serr << "An exception from delivery filter "
									"for message type "
									<< message_payload_type< Message >::subscription_type_index().name()
									<< ". Application will be aborted"
									<< std::endl;
							}
						} );
					} );
			}
#endif
	};

} /* namespace delivery_filter_templates */

template< typename Lambda >
void
agent_t::so_set_delivery_filter(
	const mbox_t & mbox,
	Lambda && lambda )
	{
		using namespace so_5::details::lambda_traits;
		using namespace delivery_filter_templates;

		using argument_type = typename argument_type_if_lambda< Lambda >::type;

		ensure_not_signal< argument_type >();

		do_set_delivery_filter(
				mbox,
				message_payload_type< argument_type >::subscription_type_index(),
				delivery_filter_unique_ptr_t{ 
					new lambda_as_filter_t< Lambda, argument_type >(
							std::move( lambda ) )
				} );
	}

//
// subscription_bind_t implementation
//
inline
subscription_bind_t::subscription_bind_t(
	agent_t & agent,
	const mbox_t & mbox_ref )
	:	m_agent( &agent )
	,	m_mbox_ref( mbox_ref )
{
}

inline subscription_bind_t &
subscription_bind_t::in(
	const state_t & state )
{
	if( !state.is_target( m_agent ) )
	{
		SO_5_THROW_EXCEPTION(
			rc_agent_is_not_the_state_owner,
			"agent doesn't own the state" );
	}

	m_states.push_back( &state );

	return *this;
}

template< typename Method_Pointer >
typename std::enable_if<
		details::is_agent_method_pointer<Method_Pointer>::value,
		subscription_bind_t & >::type
subscription_bind_t::event(
	Method_Pointer pfn,
	thread_safety_t thread_safety )
{
	using namespace details::event_subscription_helpers;

	const auto ev = preprocess_agent_event_handler( m_mbox_ref, *m_agent, pfn );
	create_subscription_for_states( 
			ev.m_msg_type,
			ev.m_handler,
			thread_safety );

	return *this;
}

template< typename Message, typename Method_Pointer >
typename std::enable_if<
		details::is_agent_method_pointer<Method_Pointer>::value,
		subscription_bind_t & >::type
subscription_bind_t::event(
	signal_indicator_t< Message >(),
	Method_Pointer pfn,
	thread_safety_t thread_safety )
{
	ensure_signal< Message >();

	using namespace details::event_subscription_helpers;

	using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

	using agent_type = typename pfn_traits::agent_type;
	using result_type = typename pfn_traits::result_type;

	// Agent must have right type.
	auto cast_result = get_actual_agent_pointer< agent_type >( *m_agent );

	const auto ev = handler< Message >(
			[cast_result, pfn]() -> result_type { return (cast_result->*pfn)(); } );

	ensure_handler_can_be_used_with_mbox( ev );
	create_subscription_for_states(
			ev.m_msg_type,
			ev.m_handler,
			thread_safety );

	return *this;
}

template<typename Lambda>
typename std::enable_if<
		details::lambda_traits::is_lambda<Lambda>::value,
		subscription_bind_t & >::type
subscription_bind_t::event(
	Lambda && lambda,
	thread_safety_t thread_safety )
{
	using namespace details::event_subscription_helpers;
	
	const auto ev = preprocess_agent_event_handler(
			m_mbox_ref,
			*m_agent,
			std::forward<Lambda>(lambda) );

	create_subscription_for_states(
			ev.m_msg_type,
			ev.m_handler,
			thread_safety );

	return *this;
}

template< class Message, class Lambda >
typename std::enable_if<
		details::lambda_traits::is_lambda<Lambda>::value,
		subscription_bind_t & >::type
subscription_bind_t::event(
	signal_indicator_t< Message > (*)(),
	Lambda && lambda,
	thread_safety_t thread_safety )
{
	const auto ev = handler< Message, Lambda >( std::forward<Lambda>(lambda) );

	ensure_handler_can_be_used_with_mbox( ev );
	create_subscription_for_states(
			ev.m_msg_type,
			ev.m_handler,
			thread_safety );

	return *this;
}

template< typename Msg >
subscription_bind_t &
subscription_bind_t::transfer_to_state(
	const state_t & target_state )
{
	/*
	 * Note. Since v.5.5.22.1 there is a new implementation of transfer_to_state.
	 * New implementation protects from loops in transfer_to_state calls.
	 * For example in the following cases:
	 * 
	 * \code
		class a_simple_case_t final : public so_5::agent_t
		{
			state_t st_base{ this, "base" };
			state_t st_disconnected{ initial_substate_of{st_base}, "disconnected" };
			state_t st_connected{ substate_of{st_base}, "connected" };

			struct message {};

		public :
			a_simple_case_t(context_t ctx) : so_5::agent_t{ctx} {
				this >>= st_base;

				st_base.transfer_to_state<message>(st_disconnected);
			}

			virtual void so_evt_start() override {
				so_5::send<message>(*this);
			}
		};

		class a_two_state_loop_t final : public so_5::agent_t
		{
			state_t st_one{ this, "one" };
			state_t st_two{ this, "two" };

			struct message {};

		public :
			a_two_state_loop_t(context_t ctx) : so_5::agent_t{ctx} {
				this >>= st_one;

				st_one.transfer_to_state<message>(st_two);
				st_two.transfer_to_state<message>(st_one);
			}

			virtual void so_evt_start() override {
				so_5::send<message>(*this);
			}
		};
	 * \endcode
	 *
	 * For such protection an additional objects with the current state
	 * of transfer_to_state operation is necessary. There will be a boolean
	 * flag in that state. When transfer_to_state will be started this
	 * flag will should be 'false'. But if it is already 'true' then there is
	 * a loop in transfer_to_state calls.
	 */

	// This is the state of transfer_to_state operation.
	struct transfer_op_state_t
	{
		agent_t * m_agent;
		mbox_id_t m_mbox_id;
		const state_t & m_target_state;
		bool m_in_progress;

		transfer_op_state_t(
			agent_t * agent,
			mbox_id_t mbox_id,
			outliving_reference_t<const state_t> tgt_state)
			:	m_agent( agent )
			,	m_mbox_id( mbox_id )
			,	m_target_state( tgt_state.get() )
			,	m_in_progress( false )
		{}
	};

	//NOTE: shared_ptr is used because C++11 doesn't support
	//initializer lists in lambda captures.
	//
	//FIXME: this should be changed to make_unique in v.5.6.0.
	auto op_state = std::make_shared< transfer_op_state_t >(
			m_agent, m_mbox_ref->id(), outliving_const(target_state) );

	auto method = [op_state](
			invocation_type_t invoke_type,
			message_ref_t & msg )
		{
			// The current transfer_to_state operation should be inactive.
			if( op_state->m_in_progress )
				SO_5_THROW_EXCEPTION( rc_transfer_to_state_loop,
						"transfer_to_state loop detected. target_state: " +
						op_state->m_target_state.query_name() +
						", current_state: " +
						op_state->m_agent->so_current_state().query_name() );

			// Activate transfer_to_state operation and make sure that it 
			// will be deactivated on return automatically.
			op_state->m_in_progress = true;
			auto in_progress_reset = details::at_scope_exit( [&op_state] {
					op_state->m_in_progress = false;
				} );

			//
			// The main logic of transfer_to_state operation.
			//
			op_state->m_agent->so_change_state( op_state->m_target_state );

			execution_demand_t demand{
					op_state->m_agent,
					nullptr, // Message limit is not actual here.
					op_state->m_mbox_id,
					typeid( Msg ),
					msg,
					invocation_type_t::event == invoke_type ?
							agent_t::get_demand_handler_on_message_ptr() :
							agent_t::get_service_request_handler_on_message_ptr()
			};

			demand.call_handler( query_current_thread_id() );
		};

	create_subscription_for_states(
			typeid( Msg ),
			method,
			thread_safety_t::unsafe );

	return *this;
}

template< typename Msg >
subscription_bind_t &
subscription_bind_t::suppress()
{
	// A method with nothing inside.
	auto method = []( invocation_type_t, message_ref_t & ) {};

	create_subscription_for_states(
			typeid( Msg ),
			method,
			thread_safety_t::safe );

	return *this;
}

template< typename Msg >
subscription_bind_t &
subscription_bind_t::just_switch_to(
	const state_t & target_state )
{
	agent_t * agent_ptr = m_agent;

	auto method = [agent_ptr, &target_state](
			invocation_type_t, message_ref_t & )
		{
			agent_ptr->so_change_state( target_state );
		};

	create_subscription_for_states(
			typeid( Msg ),
			method,
			thread_safety_t::unsafe );

	return *this;
}

inline void
subscription_bind_t::create_subscription_for_states(
	const std::type_index & msg_type,
	const event_handler_method_t & method,
	thread_safety_t thread_safety ) const
{
	if( m_states.empty() )
		// Agent should be subscribed only in default state.
		m_agent->so_create_event_subscription(
			m_mbox_ref,
			msg_type,
			m_agent->so_default_state(),
			method,
			thread_safety );
	else
		for( auto s : m_states )
			m_agent->so_create_event_subscription(
					m_mbox_ref,
					msg_type,
					*s,
					method,
					thread_safety );
}

inline void
subscription_bind_t::ensure_handler_can_be_used_with_mbox(
	const so_5::details::msg_type_and_handler_pair_t & handler ) const
{
	::so_5::details::event_subscription_helpers::ensure_handler_can_be_used_with_mbox(
			handler,
			m_mbox_ref );
}

/*
 * Implementation of template methods of state_t class.
 */
inline bool
state_t::is_active() const
{
	return m_target_agent->so_is_active_state( *this );
}

template< typename... Args >
const state_t &
state_t::event( Args&&... args ) const
{
	return this->subscribe_message_handler(
			m_target_agent->so_direct_mbox(),
			std::forward< Args >(args)... );
}

template< typename... Args >
const state_t &
state_t::event( mbox_t from, Args&&... args ) const
{
	return this->subscribe_message_handler( from,
			std::forward< Args >(args)... );
}

template< typename Signal, typename... Args >
const state_t &
state_t::event( Args&&... args ) const
{
	return this->subscribe_signal_handler< Signal >(
			m_target_agent->so_direct_mbox(),
			std::forward< Args >(args)... );
}

template< typename Signal, typename... Args >
const state_t &
state_t::event( mbox_t from, Args&&... args ) const
{
	return this->subscribe_signal_handler< Signal >(
			from,
			std::forward< Args >(args)... );
}

template< typename Msg >
bool
state_t::has_subscription( const mbox_t & from ) const
{
	return m_target_agent->so_has_subscription< Msg >( from, *this );
}

template< typename Method_Pointer >
bool
state_t::has_subscription(
	const mbox_t & from,
	Method_Pointer && pfn ) const
{
	return m_target_agent->so_has_subscription(
			from,
			*this,
			std::forward<Method_Pointer>(pfn) );
}

template< typename Msg >
void
state_t::drop_subscription( const mbox_t & from ) const
{
	m_target_agent->so_drop_subscription< Msg >( from, *this );
}

template< typename Method_Pointer >
void
state_t::drop_subscription(
	const mbox_t & from,
	Method_Pointer && pfn ) const
{
	m_target_agent->so_drop_subscription(
			from,
			*this,
			std::forward<Method_Pointer>(pfn) );
}

template< typename Msg >
const state_t &
state_t::transfer_to_state( mbox_t from, const state_t & target_state ) const
{
	m_target_agent->so_subscribe( from )
			.in( *this )
			.transfer_to_state< Msg >( target_state );

	return *this;
}

template< typename Msg >
const state_t &
state_t::transfer_to_state( const state_t & target_state ) const
{
	return this->transfer_to_state< Msg >(
			m_target_agent->so_direct_mbox(),
			target_state );
}

template< typename Msg >
const state_t &
state_t::just_switch_to( mbox_t from, const state_t & target_state ) const
{
	m_target_agent->so_subscribe( from )
			.in( *this )
			.just_switch_to< Msg >( target_state );

	return *this;
}

template< typename Msg >
const state_t &
state_t::just_switch_to( const state_t & target_state ) const
{
	return this->just_switch_to< Msg >(
			m_target_agent->so_direct_mbox(),
			target_state );
}

template< typename Msg >
const state_t &
state_t::suppress() const
{
	return this->suppress< Msg >( m_target_agent->so_direct_mbox() );
}

template< typename Msg >
const state_t &
state_t::suppress( mbox_t from ) const
{
	m_target_agent->so_subscribe( from )
			.in( *this )
			.suppress< Msg >();

	return *this;
}

template< typename Method_Pointer >
typename std::enable_if<
		details::is_agent_method_pointer<Method_Pointer>::value,
		state_t & >::type
state_t::on_enter( Method_Pointer pfn )
{
	using namespace details::event_subscription_helpers;

	using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

	// Agent must have right type.
	auto cast_result =
			get_actual_agent_pointer<
					typename pfn_traits::agent_type >(
			*m_target_agent );

	return this->on_enter( [cast_result, pfn]() { (cast_result->*pfn)(); } );
}

template< typename Method_Pointer >
typename std::enable_if<
		details::is_agent_method_pointer<Method_Pointer>::value,
		state_t & >::type
state_t::on_exit( Method_Pointer pfn )
{
	using namespace details::event_subscription_helpers;

	using pfn_traits = details::is_agent_method_pointer<Method_Pointer>;

	// Agent must have right type.
	auto cast_result =
			get_actual_agent_pointer<
					typename pfn_traits::agent_type >(
			*m_target_agent );

	return this->on_exit( [cast_result, pfn]() { (cast_result->*pfn)(); } );
}

template< typename... Args >
const state_t &
state_t::subscribe_message_handler(
	const mbox_t & from,
	Args&&... args ) const
{
	m_target_agent->so_subscribe( from ).in( *this )
			.event( std::forward< Args >(args)... );

	return *this;
}

template< typename Signal, typename... Args >
const state_t &
state_t::subscribe_signal_handler(
	const mbox_t & from,
	Args&&... args ) const
{
	m_target_agent->so_subscribe( from )
			.in( *this ).event(
					so_5::signal< Signal >,
					std::forward< Args >(args)... );

	return *this;
}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A shortcat for switching the agent state.
 *
 * \par Usage example.
	\code
	class my_agent : public so_5::agent_t
	{
		const so_5::state_t st_normal = so_make_state();
		const so_5::state_t st_error = so_make_state();
		...
	public :
		virtual void so_define_agent() override
		{
			this >>= st_normal;

			st_normal.handle( [=]( const msg_failure & evt ) {
				this >>= st_error;
				...
			});
			...
		};
		...
	};
	\endcode
 */
inline void
operator>>=( agent_t * agent, const state_t & new_state )
{
	agent->so_change_state( new_state );
}

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::exception_reaction_t
 * instead.
 */
using exception_reaction_t = so_5::exception_reaction_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::abort_on_exception
 * instead.
 */
const so_5::exception_reaction_t abort_on_exception = so_5::abort_on_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::shutdown_sobjectizer_on_exception instead.
 */
const so_5::exception_reaction_t shutdown_sobjectizer_on_exception = so_5::shutdown_sobjectizer_on_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::deregister_coop_on_exception instead.
 */
const so_5::exception_reaction_t deregister_coop_on_exception = so_5::deregister_coop_on_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ignore_exception
 * instead.
 */
const so_5::exception_reaction_t ignore_exception = so_5::ignore_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::inherit_exception_reaction
 * instead.
 */
const so_5::exception_reaction_t inherit_exception_reaction = so_5::inherit_exception_reaction;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::subscription_bind_t
 * instead.
 */
using subscription_bind_t = so_5::subscription_bind_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::agent_t
 * instead.
 */
using agent_t = so_5::agent_t;

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

