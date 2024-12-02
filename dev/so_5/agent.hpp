/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for agents.
*/

#pragma once

#include <so_5/compiler_features.hpp>
#include <so_5/declspec.hpp>
#include <so_5/types.hpp>
#include <so_5/current_thread_id.hpp>
#include <so_5/atomic_refcounted.hpp>
#include <so_5/spinlocks.hpp>
#include <so_5/outliving.hpp>

#include <so_5/exception.hpp>
#include <so_5/error_logger.hpp>

#include <so_5/details/rollback_on_exception.hpp>
#include <so_5/details/at_scope_exit.hpp>

#include <so_5/fwd.hpp>

#include <so_5/agent_ref_fwd.hpp>
#include <so_5/agent_context.hpp>
#include <so_5/agent_identity.hpp>
#include <so_5/mbox.hpp>
#include <so_5/agent_state_listener.hpp>
#include <so_5/event_queue.hpp>
#include <so_5/subscription_storage_fwd.hpp>
#include <so_5/handler_makers.hpp>
#include <so_5/message_handler_format_detector.hpp>
#include <so_5/coop_handle.hpp>

#include <so_5/disp_binder.hpp>

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

//
// exception_reaction_t
//
/*!
 * \brief A reaction of SObjectizer to an exception from agent event.
 *
 * \since v.5.2.3
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
	 * \brief Exception reaction should be inherited from SO Environment.
	 *
	 * \since v.5.3.0
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
		 *
		 * \since v.5.5.14
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<
						details::method_arity::unary,
						Method_Pointer>::value,
				subscription_bind_t & >::type
		event(
			//! Event handling method.
			Method_Pointer pfn,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
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
		 *
		 * \since v.5.3.0
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
		 *
		 * \since v.5.5.15
		 */
		template< typename Msg >
		subscription_bind_t &
		transfer_to_state(
			const state_t & target_state );

		/*!
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
		 *
		 * \since v.5.5.15
		 */
		template< typename Msg >
		subscription_bind_t &
		suppress();

		/*!
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
		 *
		 * \since v.5.5.15
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
		 * \brief Type of vector of states.
		 *
		 * \since v.5.3.0
		 */
		typedef std::vector< const state_t * > state_vector_t;

		/*!
		 * \brief States of agents the event to be subscribed in.
		 *
		 * \since v.5.3.0
		 */
		state_vector_t m_states;

		/*!
		 * \brief Create subscription of event for all states.
		 *
		 * \since v.5.3.0
		 */
		void
		create_subscription_for_states(
			const std::type_index & msg_type,
			const event_handler_method_t & method,
			thread_safety_t thread_safety,
			event_handler_kind_t handler_kind ) const;

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
		 * \since v.5.5.19
		 */
		void
		ensure_handler_can_be_used_with_mbox(
			const so_5::details::msg_type_and_handler_pair_t & handler ) const;
};

/*!
 * \brief Internal namespace with details of agent_t implementation.
 *
 * \attention
 * Nothing from that namespace can be used in user code. All of this is an
 * implementation detail and is subject to change without any prior notice.
 *
 * \since v.5.8.0
 */
namespace impl::agent_impl
{

/*!
 * \brief A helper class for temporary setting and then dropping
 * the ID of the current working thread.
 *
 * \note New working thread_id is set only if it is not an
 * null thread_id.
 *
 * \since v.5.4.0
 */
struct working_thread_id_sentinel_t
	{
		so_5::current_thread_id_t & m_id;

		working_thread_id_sentinel_t(
			so_5::current_thread_id_t & id_var,
			so_5::current_thread_id_t value_to_set )
			:	m_id( id_var )
			{
				if( value_to_set != null_current_thread_id() )
					m_id = value_to_set;
			}
		~working_thread_id_sentinel_t()
			{
				if( m_id != null_current_thread_id() )
					m_id = null_current_thread_id();
			}
	};

} /* namespace impl::agent_impl */

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
		// Since v.5.5.20:
		return_type evt_handler( mhood_t< Message > msg ) const;
		return_type evt_handler( const mhood_t< Message > & msg ) const;
		return_type evt_handler( const Message & msg ) const;
		return_type evt_handler( Message msg ) const;
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
		void evt_handler( mhood_t< Message > msg );
		void return_type evt_handler( mhood_t< Message > msg ) const;
		void return_type evt_handler( const mhood_t< Message > & msg );
		void return_type evt_handler( const mhood_t< Message > & msg ) const;
		void return_type evt_handler( const Message & msg );
		void return_type evt_handler( const Message & msg ) const;
		void return_type evt_handler( Message msg );
		void return_type evt_handler( Message msg ) const;
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

	<b>Work thread identification</b>

	Since v.5.4.0 some operations for agent are enabled only on agent's
	work thread. They are:
	- subscription management operations (creation or dropping);
	- changing agent's state.

	Work thread for an agent is defined as follows:
	- before invocation of so_define_agent() the work thread is a
	  thread on which agent is created (id of that thread is detected in
	  agent's constructor);
	- during cooperation registration the working thread is a thread on
	  which so_environment::register_coop() is working;
	- after successful agent registration the work thread for it is
	  specified by the dispatcher.

	\note Some dispatchers could provide several work threads for
	an agent. In such case there would not be work thread id. And
	operations like changing agent state or creation of subscription
	would be prohibited after agent registration.

	<b>Accessing dispatcher binders</b>

	Since v.5.8.1 there are two methods that allow to get a dispatcher binder
	related to the agent or agent's coop:

	- so_this_agent_disp_binder(). It returns the dispatcher binder that is
	  used for binding the agent itself;
	- so_this_coop_disp_binder(). It returns the dispatcher binder that is the
	  default disp binder for the agent's coop.

	Please note that binders returned by so_this_agent_disp_binder() and
	so_this_coop_disp_binder() may be different binders.
*/
class SO_5_TYPE agent_t
	:	private atomic_refcounted_t
	,	public message_limit::message_limit_methods_mixin_t
	,	public name_for_agent_methods_mixin_t
{
		friend class subscription_bind_t;
		friend class state_t;

		friend class so_5::impl::mpsc_mbox_t;
		friend class so_5::impl::state_switch_guard_t;
		friend class so_5::impl::internal_agent_iface_t;

		friend class so_5::enveloped_msg::impl::agent_demand_handler_invoker_t;

		template< typename T >
		friend class intrusive_ptr_t;

	public:
		/*!
		 * \brief Short alias for agent_context.
		 *
		 * \since v.5.5.4
		 */
		using context_t = so_5::agent_context_t;
		/*!
		 * \brief Short alias for %so_5::state_t.
		 *
		 * \since v.5.5.13
		 */
		using state_t = so_5::state_t;
		/*!
		 * \brief Short alias for %so_5::mhood_t.
		 *
		 * \since v.5.5.14
		 */
		template< typename T >
		using mhood_t = so_5::mhood_t< T >;
		/*!
		 * \brief Short alias for %so_5::mutable_mhood_t.
		 *
		 * \since v.5.5.19
		 */
		template< typename T >
		using mutable_mhood_t = so_5::mutable_mhood_t< T >;
		/*!
		 * \brief Short alias for %so_5::initial_substate_of.
		 *
		 * \since v.5.5.15
		 */
		using initial_substate_of = so_5::initial_substate_of;
		/*!
		 * \brief Short alias for %so_5::substate_of.
		 *
		 * \since v.5.5.15
		 */
		using substate_of = so_5::substate_of;
		/*!
		 * \brief Short alias for %so_5::state_t::history_t::shallow.
		 *
		 * \since v.5.5.15
		 */
		static constexpr const state_t::history_t shallow_history =
				state_t::history_t::shallow;
		/*!
		 * \brief Short alias for %so_5::state_t::history_t::deep.
		 *
		 * \since v.5.5.15
		 */
		static constexpr const state_t::history_t deep_history =
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
		 *
		 * \since v.5.5.3
		 */
		agent_t(
			environment_t & env,
			agent_tuning_options_t tuning_options );

		/*!
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
		 auto coop = env.make_coop();
		 auto a = coop->make_agent< my_agent >();
		 auto b = coop->make_agent< my_more_specific_agent >();
		 * \endcode
		 *
		 * \since v.5.5.4
		 */
		explicit agent_t( context_t ctx );

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
		 * \since v.5.5.15
		 */
		bool
		so_is_active_state( const state_t & state_to_check ) const noexcept;

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
		 *
		 * \note
		 * This method is noexcept since v.5.8.0.
		 *
		 * \since v.5.2.3
		 */
		virtual exception_reaction_t
		so_exception_reaction() const noexcept;

		/*!
		 * \brief Switching agent to special state in case of unhandled
		 * exception.
		 *
		 * \note
		 * Since v.5.7.3 it's implemented via so_deactivate_agent().
		 *
		 * \attention
		 * The method is not noexcept, it can throw an exception. So additional
		 * care has to be taken when it's called in catch-block and/or in
		 * noexcept contexts.
		 *
		 * \since 5.2.3
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
			const std::type_index & msg_type,
			const message_ref_t & message )
			{
				agent.push_event( limit, mbox_id, msg_type, message );
			}

		/*!
		 * \brief Get the agent's direct mbox.
		 *
		 * \since v.5.4.0
		 */
		const mbox_t &
		so_direct_mbox() const;

		/*!
		 * \brief Create a new direct mbox for that agent.
		 *
		 * This method creates a new MPSC mbox which is connected
		 * with that agent. Only agent for that so_make_new_direct_mbox()
		 * has been called can make subscriptions for a new mbox.
		 *
		 * Note. The new mbox doesn't replaces the standard direct mbox
		 * for the agent. Old direct mbox is still here and can still be used
		 * for sending messages to the agent. But new mbox is not related
		 * to the old direct mbox: they are different mboxes and can be used
		 * for different subscriptions.
		 * For example:
		 * \code
		 * class my_agent final : public so_5::agent_t {
		 * 	...
		 * 	void so_evt_start() override {
		 * 		so_subscribe_self().event( [](mhood_t<hello>) {
		 * 			std::cout << "hello from the direct mbox" << std::endl;
		 * 		} );
		 *
		 * 		const auto new_mbox = so_make_new_direct_mbox();
		 * 		so_subscribe( new_mbox ).event( [](mhood_t<hello) {
		 * 			std::cout << "hello from a new mbox" << std::endl;
		 * 		}
		 *
		 * 		so_5::send<hello>(*this);
		 * 		so_5::send<hello>(new_mbox);
		 * 	}
		 * };
		 * \endcode
		 * The output will be:
		 \verbatim
		 hello from the direct mbox
		 hello from a new mbox
		 \endverbatim
		 *
		 * \since v.5.6.0
		 */
		mbox_t
		so_make_new_direct_mbox();

		/*!
		 * \brief Create tuning options object with default values.
		 *
		 * \since v.5.5.3
		 */
		inline static agent_tuning_options_t
		tuning_options()
		{
			return agent_tuning_options_t();
		}

		/*!
		 * \brief Helper for creation a custom direct mbox factory.
		 *
		 * Usage example:
		 * \code
		 * class my_agent : public so_5::agent_t {
		 * 	...
		 * public:
		 * 	my_agent( context_t ctx )
		 * 		:	so_5::agent_t{ ctx + custom_direct_mbox_factory(
		 * 				[]( so_5::partially_constructed_agent_ptr_t agent_ptr,
		 * 				    so_5::mbox_t actual_mbox )
		 * 				{
		 * 					return so_5::mbox_t{ new my_custom_mbox{ agent_ptr.ptr(), std::move(actual_mbox) } };
		 * 				} )
		 * 			}
		 * 	{...}
		 *
		 * 	...
		 * };
		 * \endcode
		 *
		 * \since v.5.7.4
		 */
		template< typename Lambda >
		[[nodiscard]]
		static custom_direct_mbox_factory_t
		custom_direct_mbox_factory( Lambda && lambda )
		{
			return { std::forward<Lambda>(lambda) };
		}

	protected:
		/*!
		 * \name Accessing the default state.
		 * \{
		 */

		//! Access to the agent's default state.
		const state_t &
		so_default_state() const;
		/*!
		 * \}
		 */

	public : /* Note: since v.5.5.1 method so_change_state() is public */

		/*!
		 * \name Changing agent's state.
		 * \{
		 */
		//! Change the current state of the agent.
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

			\attention
			This method has to be called from a worker thread assigned
			to the agent by the dispatcher. This method can't be called from
			thread_safe event-handlers because so_change_state() modifies
			the state of the agent.
		*/
		void
		so_change_state(
			//! New agent state.
			const state_t & new_state );

		/*!
		 * \brief Deactivate the agent.
		 *
		 * This method deactivates the agent:
		 *
		 * - drops all agent's subscriptions (including deadletter handlers) and
		 *   delivery filters;
		 * - switches the agent to a special state in that the agent does nothing
		 *   and just waits the deregistration.
		 *
		 * Sometimes it is necessary to mark an agent as 'failed'. Such an agent
		 * shouldn't process anything and the only thing that is allowed
		 * is waiting for the deregistration. For example:
		 *
		 * \code
		 * class some_agent final : public so_5::agent_t
		 * {
		 * 	state_t st_working{ this, "working" };
		 * 	state_t st_failed{ this, "failed" };
		 * 	...
		 * 	void on_enter_st_failed()
		 * 	{
		 * 		// Notify some supervisor about the failure.
		 * 		// It will deregister the whole cooperation with failed agent.
		 * 		so_5::send<msg_failure>( supervisor_mbox(), ... );
		 * 	}
		 * 	...
		 * 	void so_define_agent() override
		 * 	{
		 * 		this >>= st_working;
		 *
		 * 		st_failed.on_enter( &some_agent::on_enter_st_failed );
		 *
		 * 		...
		 * 	}
		 *
		 * 	void evt_some_event(mhood_t<some_msg> cmd)
		 * 	{
		 * 		try
		 * 		{
		 * 			do_some_processing_of(*cmd);
		 * 		}
		 * 		catch(...)
		 * 		{
		 * 			// Processing failed, agent can't continue work normally.
		 * 			// Have to switch it to the failed state and wait for
		 * 			// the deregistration.
		 * 			this >>= st_failed;
		 * 		}
		 * 	}
		 * 	...
		 * };
		 * \endcode
		 *
		 * This approach works but has a couple of drawbacks:
		 *
		 * - it's necessary to define a separate state for an agent (like
		 *   st_failed shown above);
		 * - agent still has all its subscriptions. It means that messages will
		 *   be delivered to the agent's event queue and dispatched by the
		 *   agent's dispatcher. They won't be handled because there are no
		 *   subscriptions in a state like st_failed, but message dispatching
		 *   will consume some resources, although the agent is a special
		 *   'failed' state.
		 *
		 * To cope with those drawbacks so_deactivate_agent was introduced in
		 * v.5.7.3. That method drops all agent's subscriptions (including
		 * deadletter handlers and delivery filters) and switches the agent to a
		 * special hidden state in that the agent doesn't handle anything.
		 *
		 * The example above now can be rewritten that way:
		 * \code
		 * class some_agent final : public so_5::agent_t
		 * {
		 * 	state_t st_working{ this, "working" };
		 * 	...
		 * 	void switch_to_failed_state()
		 * 	{
		 * 		// Notify some supervisor about the failure.
		 * 		// It will deregister the whole cooperation with failed agent.
		 * 		so_5::send<msg_failure>( supervisor_mbox(), ... );
		 * 		// Deactivate the agent.
		 * 		so_deactivate_agent();
		 * 	}
		 * 	...
		 * 	void so_define_agent() override
		 * 	{
		 * 		this >>= st_working;
		 * 		...
		 * 	}
		 *
		 * 	void evt_some_event(mhood_t<some_msg> cmd)
		 * 	{
		 * 		try
		 * 		{
		 * 			do_some_processing_of(*cmd);
		 * 		}
		 * 		catch(...)
		 * 		{
		 * 			// Processing failed, agent can't continue work normally.
		 * 			// Have to switch it to the failed state and wait for
		 * 			// the deregistration.
		 * 			switch_to_failed_state();
		 * 		}
		 * 	}
		 * 	...
		 * };
		 * \endcode
		 *
		 * \note
		 * The method uses so_change_state(), so it has all requirements of
		 * so_change_state(). Because the agent state will be changed,
		 * so_deactivate_state() has to be called on the working thread
		 * assigned to the agent by the dispatcher, and
		 * so_deactivate_state() can't be invoked from a thread_safe event
		 * handler.
		 *
		 * \attention
		 * The method is not noexcept, it can throw an exception. So additional
		 * care has to be taken when it's called in catch-block and/or in
		 * noexcept contexts.
		 *
		 * \since v.5.7.3
		 */
		void
		so_deactivate_agent();
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
		 *
		 * \since v.5.5.1
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
		 *
		 * \note
		 * Parameter \a handler_kind was introduced in v.5.7.0.
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
			thread_safety_t thread_safety,
			//! Kind of that event handler.
			event_handler_kind_t handler_kind );

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
		 * \since v.5.5.21
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
		 *
		 * \since v.5.2.3
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<
						details::method_arity::unary,
						Method_Pointer>::value,
				void >::type
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state,
			Method_Pointer /*pfn*/ )
		{
			using pfn_traits = details::is_agent_method_pointer<
					details::method_arity::unary, Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type >::type;

			do_drop_subscription( mbox,
					message_payload_type< message_type >::subscription_type_index(),
					target_state );
		}

		/*!
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
		 *
		 * \since v.5.5.3
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
		 *
		 * \since v.5.2.3
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<
						details::method_arity::unary,
						Method_Pointer>::value,
				void >::type
		so_drop_subscription(
			const mbox_t & mbox,
			Method_Pointer /*pfn*/ )
		{
			using pfn_traits = details::is_agent_method_pointer<
					details::method_arity::unary, Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type >::type;

			do_drop_subscription(
					mbox,
					message_payload_type< message_type >::subscription_type_index(),
					so_default_state() );
		}

		/*!
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
		 *
		 * \since v.5.5.3
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
		 *
		 * \since v.5.2.3
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<
						details::method_arity::unary,
						Method_Pointer>::value,
				void >::type
		so_drop_subscription_for_all_states(
			const mbox_t & mbox,
			Method_Pointer /*pfn*/ )
		{
			using pfn_traits = details::is_agent_method_pointer<
					details::method_arity::unary, Method_Pointer>;

			using message_type =
					typename details::message_handler_format_detector<
							typename pfn_traits::argument_type >::type;

			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< message_type >::subscription_type_index() );
		}

		/*!
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
		 *
		 * \since v.5.5.3
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
		 * \since v.5.5.19.5
		 */
		template< class Message >
		bool
		so_has_subscription(
			//! A mbox from which message/signal of type \a Message is expected.
			const mbox_t & mbox,
			//! A target state for the subscription.
			const state_t & target_state ) const noexcept
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
		 * \since v.5.5.19.5
		 */
		template< class Message >
		bool
		so_has_subscription(
			//! A mbox from which message/signal of type \a Message is expected.
			const mbox_t & mbox ) const noexcept
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
		 * \since v.5.5.19.5
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<
						details::method_arity::unary,
						Method_Pointer>::value,
				bool >::type
		so_has_subscription(
			//! A mbox from which message/signal is expected.
			const mbox_t & mbox,
			//! A target state for the subscription.
			const state_t & target_state,
			Method_Pointer /*pfn*/ ) const noexcept
		{
			using pfn_traits = details::is_agent_method_pointer<
					details::method_arity::unary, Method_Pointer>;

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
		 * \since v.5.5.19.5
		 */
		template< typename Method_Pointer >
		typename std::enable_if<
				details::is_agent_method_pointer<
						details::method_arity::unary,
						Method_Pointer>::value,
				bool >::type
		so_has_subscription(
			//! A mbox from which message/signal is expected.
			const mbox_t & mbox,
			Method_Pointer /*pfn*/ ) const noexcept
		{
			using pfn_traits = details::is_agent_method_pointer<
					details::method_arity::unary, Method_Pointer>;

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
		 * \since v.5.5.21
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
		 * \since v.5.5.21
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
		 * void deadletter_handler(message_type);
		 * void deadletter_handler(const message_type &);
		 * void deadletter_handler(mhood_t<message_type>);
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
		 * 			[](mhood_t<status_request> cmd) {
		 * 				so_5::send<current_status>(cmd->reply_mbox, "workind");
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
		 * \since v.5.5.21
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
		 * \since v.5.5.21
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
		 * \since v.5.5.21
		 */
		template< typename Message >
		bool
		so_has_deadletter_handler(
			//! A mbox from which the message is expected.
			const so_5::mbox_t & mbox ) const noexcept
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
		 * \brief A correct initiation of so_define_agent method call.
		 *
		 * Before the actual so_define_agent() method it is necessary
		 * to temporary set working thread id. And then drop this id
		 * to non-actual value after so_define_agent() return.
		 *
		 * Because of that this method must be called during cooperation
		 * registration procedure instead of direct call of so_define_agent().
		 *
		 * \since v.5.4.0
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
				so_5::coop_unique_holder_t coop = so_environment().make_coop();

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
		so_environment() const noexcept;

		/*!
		 * \brief Get a handle of agent's coop.
		 *
		 * \note
		 * This method is a replacement for so_coop_name() method
		 * from previous versions of SObjectizer-5.
		 *
		 * \attention
		 * If this method is called when agent is not registered (e.g.
		 * there is no coop for agent) then this method will throw.
		 *
		 * Usage example:
		 * \code
		 * class parent final : public so_5::agent_t {
		 * 	...
		 * 	void so_evt_start() override {
		 * 		// Create a child coop.
		 * 		auto coop = so_environment().make_coop(
		 * 			// We as a parent coop.
		 * 			so_coop() );
		 * 		...; // Fill the coop.
		 * 		so_environment().register_coop( std::move(coop) );
		 * 	}
		 * };
		 * \endcode
		 *
		 * \since v.5.6.0
		 */
		[[nodiscard]]
		coop_handle_t
		so_coop() const;

		/*!
		 * \brief Binding agent to the dispatcher.
		 *
		 * This is an actual start of agent's work in SObjectizer.
		 *
		 * \note
		 * This method was a de-facto noexcept in previous versions of
		 * SObjectizer. But didn't marked as noexcept because of need of
		 * support old C++ compilers. Since v.5.6.0 it is officially noexcept.
		 *
		 * \since v.5.4.0
		 */
		void
		so_bind_to_dispatcher(
			//! Actual event queue for an agent.
			event_queue_t & queue ) noexcept;

		/*!
		 * \brief Create execution hint for the specified demand.
		 *
		 * The hint returned is intendent for the immediately usage.
		 * It must not be stored for the long time and used sometime in
		 * the future. It is because internal state of the agent
		 * can be changed and some references from hint object to
		 * agent's internals become invalid.
		 *
		 * \since v.5.4.0
		 */
		static execution_hint_t
		so_create_execution_hint(
			//! Demand for execution of event handler.
			execution_demand_t & demand );

		/*!
		 * \brief A helper method for deregistering agent's coop.
		 *
		 * Usage example:
		 * \code
		 * class demo : public so_5::agent_t {
		 * ...
		 *     void on_some_event(mhood_t<some_msg> cmd) {
		 *         try {
		 *             ... // Some processing.
		 *             if(no_more_work_left())
		 *                 // Normal deregistration of the coop.
		 *                 so_deregister_agent_coop_normally();
		 *         }
		 *         catch(...) {
		 *             // Some error.
		 *             // Deregister the coop with special 'exit code'.
		 *             so_deregister_agent_coop(so_5::dereg_reason::user_defined_reason+10);
		 *         }
		 *     }
		 * };
		 * \endcode
		 *
		 * \since v.5.4.0
		 */
		void
		so_deregister_agent_coop( int dereg_reason );

		/*!
		 * \brief A helper method for deregistering agent's coop
		 * in case of normal deregistration.
		 *
		 * \note It is just a shorthand for:
			\code
			so_deregister_agent_coop( so_5::dereg_reason::normal );
			\endcode
		 *
		 * \since v.5.4.0
		 */
		void
		so_deregister_agent_coop_normally();

		/*!
		 * \name Methods for dealing with message delivery filters.
		 * \{
		 */
		/*!
		 * \brief Set a delivery filter.
		 *
		 * \note
		 * Since v.5.7.4 it can be used for mutable messages too (if mbox is MPSC mbox).
		 * In that case \a Message should be in form `so_5::mutable_msg<Message>`.
		 *
		 * \tparam Message type of message to be filtered.
		 *
		 * \since v.5.5.5
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
		 *
		 * \since v.5.5.5
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
		 * \brief Set a delivery filter for a mutable message.
		 *
		 * \tparam Lambda type of lambda-function or functional object which
		 * must be used as message filter.
		 *
		 * \note
		 * The content of the message will be passed to delivery-filter
		 * lambda-function by a const reference.
		 *
		 * \par Usage sample:
		 \code
		 void my_agent::so_define_agent() {
		 	so_set_delivery_filter_for_mutable_msg( temp_sensor,
				[]( const current_temperature & msg ) {
					return !is_normal_temperature( msg );
				} );
			...
		 }
		 \endcode
		 *
		 * \since v.5.7.4
		 */
		template< typename Lambda >
		void
		so_set_delivery_filter_for_mutable_msg(
			//! Message box from which message is expected.
			//! This must be MPMC-mbox.
			const mbox_t & mbox,
			//! Delivery filter as lambda-function or functional object.
			Lambda && lambda );

		/*!
		 * \brief Drop a delivery filter.
		 *
		 * Usage example:
		 * \code
		 * // For a case of an immutable message.
		 * void some_agent::some_event(mhood_t<my_message> cmd) {
		 * 	... // Some actions.
		 * 	// Now we want to drop the subscription and the delivery
		 * 	// filter for this message.
		 * 	so_drop_subscription_for_all_states<my_message>(source_mbox);
		 * 	so_drop_delivery_filter<my_message>(source_mbox);
		 * }
		 *
		 * // For a case of a mutable message.
		 * void some_agent::some_event(mutable_mhood_t<my_message> cmd) {
		 * 	... // Some actions.
		 * 	// Now we want to drop the subscription and the delivery
		 * 	// filter for this message.
		 * 	so_drop_subscription_for_all_states<so_5::mutable_msg<my_message>>(source_mbox);
		 * 	so_drop_delivery_filter<so_5::mutable_msg<my_message>>(source_mbox);
		 * }
		 * \endcode
		 *
		 * \tparam Message type of message filtered.
		 *
		 * \since v.5.5.5
		 */
		template< typename Message >
		void
		so_drop_delivery_filter(
			//! Message box to which delivery filter was set.
			//! This must be MPMC-mbox.
			const mbox_t & mbox ) noexcept
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
		 * \brief Get the priority of the agent.
		 *
		 * \since v.5.5.8
		 */
		[[nodiscard]]
		priority_t
		so_priority() const noexcept
			{
				return m_priority;
			}
		/*!
		 * \}
		 */

		/*!
		 * \brief Helper method that allows to run a block of code as
		 * non-thread-safe event handler.
		 *
		 * \attention
		 * This is a low-level method. Using it may destroy all thread-safety
		 * guarantees provided by SObjectizer. Please use it only when you know
		 * what your are doing. All responsibility rests with the user.
		 *
		 * Use of this method may be necessary when an agent is bound to a
		 * special dispatcher that runs not only the agent's event-handlers, but
		 * also other callbacks on the same worker thread.
		 *
		 * A good example of such a dispatcher is so5extra's asio_one_thread
		 * dispatcher. It guarantees that an IO completion handler is called on
		 * the same worker thread as agent's event-handlers. For example:
		 * \code
		 * class agent_that_uses_asio : public so_5::agent_t
		 * {
		 * 	state_t st_not_ready{this};
		 * 	state_t st_ready{this};
		 *
		 * 	asio::io_context & io_ctx_;
		 *
		 * public:
		 * 	agent_that_uses_asio(context_t ctx, asio::io_context & io_ctx)
		 * 		:	so_5::agent_t{std::move(ctx)}
		 * 		,	io_ctx_{io_ctx}
		 * 	{}
		 * 	...
		 * 	void so_define_agent() override
		 * 	{
		 * 		st_not_ready.activate();
		 * 		...
		 * 	}
		 *
		 * 	void so_evt_start() override
		 * 	{
		 * 		auto resolver = std::make_shared<asio::ip::tcp::resolver>(io_ctx_);
		 * 		resolver->async_resolve("some.host.name", "",
		 * 				asio::ip::tcp::numeric_service | asio::ip::tcp::address_configured,
		 * 				// IO completion handler to be run on agent's worker thread.
		 * 				[resolver, this](auto ec, auto results) {
		 * 					// It's necessary to wrap the block of code, otherwise
		 * 					// modification of the agent's state (or managing of subscriptions)
		 * 					// will be prohibited because SObjectizer doesn't see
		 * 					// the IO completion handler as event handler.
		 * 					so_low_level_exec_as_event_handler( [&]() {
		 * 							...
		 * 							st_ready.activate();
		 * 						});
		 * 				});
		 * 	}
		 * }
		 * \endcode
		 *
		 * \attention
		 * Using this method inside a running event-handler (non-thread-safe and
		 * especially thread-safe) is undefined behavior. SObjectizer can't check
		 * such a case without a significant performance penalty, so there won't
		 * be any warnings or errors from SObjectizer's side, anything can
		 * happen.
		 *
		 * \since v.5.8.0
		 */
		template< typename Lambda >
		decltype(auto)
		so_low_level_exec_as_event_handler(
			Lambda && lambda ) noexcept( noexcept(lambda()) )
			{
				impl::agent_impl::working_thread_id_sentinel_t sentinel{
						m_working_thread_id,
						query_current_thread_id()
					};

				return lambda();
			}

		/*!
		 * \brief Returns the dispatcher binder that is used for binding this
		 * agent.
		 *
		 * \attention
		 * It's safe to use this method only while the agent is registered
		 * in a SObjectizer Environment -- from the start of so_evt_start() until
		 * the completion so_evt_finish(). The calling of this method when agent
		 * it not registered (e.g. before the invocation of so_evt_start() or
		 * after the completion of so_evt_finish()) may lead to undefined behavior.
		 *
		 * This method is intended to simplify creation of children cooperations:
		 * \code
		 * void parent_agent::evt_some_command(mhood_t<msg_command> cmd) {
		 * 	...
		 * 	// A new child coop has to be created and bound to the same
		 * 	// dispatcher as the parent agent.
		 * 	so_5::introduce_child_coop( *this,
		 * 		// Get the binder of the parent.
		 * 		so_this_agent_disp_binder(),
		 * 		[&](so_5::coop_t & coop) {
		 * 			... // Creation of children agents.
		 * 		} );
		 * }
		 * \endcode
		 *
		 * \since v.5.8.1
		 */
		[[nodiscard]]
		disp_binder_shptr_t
		so_this_agent_disp_binder() const
			{
				return m_disp_binder;
			}

		/*!
		 * \brief Returns the dispatcher binder that is used as the default
		 * binder for the agent's coop.
		 *
		 * \attention
		 * It's safe to use this method only while the agent is registered
		 * in a SObjectizer Environment -- from the start of so_evt_start() until
		 * the completion so_evt_finish(). The calling of this method when agent
		 * it not registered (e.g. before the invocation of so_evt_start() or
		 * after the completion of so_evt_finish()) may lead to undefined behavior.
		 *
		 * This method is intended to simplify creation of children cooperations:
		 * \code
		 * void parent_agent::evt_some_command(mhood_t<msg_command> cmd) {
		 * 	...
		 * 	// A new child coop has to be created and bound to the same
		 * 	// dispatcher as the parent agent.
		 * 	so_5::introduce_child_coop( *this,
		 * 		// Get the binder of the parent's coop.
		 * 		so_this_coop_disp_binder(),
		 * 		[&](so_5::coop_t & coop) {
		 * 			... // Creation of children agents.
		 * 		} );
		 * }
		 * \endcode
		 *
		 * \note
		 * This method may return a different binder that so_this_agent_disp_binder()
		 * in a case when the agent was bound by a separate dispatcher. For example:
		 * \code
		 * // The parent coop will use thread_pool dispatcher as
		 * // the default dispatcher.
		 * env.introduce_coop(
		 * 	so_5::disp::thread_pool::make_dispatcher( env, 8u )
		 * 		.binder( []( auto & params ) {
		 * 				// Every agent will have a separate event queue.
		 * 				params.fifo( so_5::disp::thread_pool::fifo_t::individual );
		 * 			} ),
		 * 	[&]( so_5::coop_t & coop ) {
		 * 		// The parent agent itself will use a separate dispatcher.
		 * 		coop.make_agent_with_binder< parent_agent >(
		 * 			so_5::disp::one_thread::make_dispatcher( env ).binder(),
		 * 			... );
		 *
		 * 		... // Creation of other agents.
		 * 	} );
		 * \endcode
		 * In that case use of so_this_agent_disp_binder() instead of
		 * so_this_coop_disp_binder() will bind children agents to the
		 * parent's one_thread dispatcher instead of coop's thread_pool
		 * dispatcher.
		 *
		 * \since v.5.8.1
		 */
		[[nodiscard]]
		disp_binder_shptr_t
		so_this_coop_disp_binder() const;

		/*!
		 * \brief Get an optional name of the agent.
		 *
		 * If agent has the name then a reference to this name will be returned.
		 * Otherwise a small object with a pointer to agent will be returned.
		 *
		 * The result can be printed to std::ostream or converted into a string:
		 * \code
		 * class my_agent final : public so_5::agent_t
		 * {
		 * ...
		 * 	void so_evt_start() override
		 * 	{
		 * 		std::cout << so_agent_name() << ": started" << std::endl;
		 * 		...
		 * 		so_5::send<std::string>(some_mbox, so_agent_name().to_string());
		 * 	}
		 * 	void so_evt_finished() override
		 * 	{
		 * 		std::cout << so_agent_name() << ": stopped" << std::endl;
		 * 	}
		 * ...
		 * }
		 * \endcode
		 *
		 * \attention
		 * This method returns a lightweight object that just holds a reference
		 * to the agent's name (or a pointer to the agent). This object should
		 * not be stored for the long time, because the references/pointers it
		 * holds may become invalid. If you have to store the agent name for
		 * a long time please convert the returned value into std::string and
		 * store the resulting std::string object.
		 *
		 * \since v.5.8.2
		 */
		[[nodiscard]]
		agent_identity_t
		so_agent_name() const noexcept;

	private:
		const state_t st_default{ self_ptr(), "<DEFAULT>" };

		//! Current agent state.
		const state_t * m_current_state_ptr;

		/*!
		 * \brief Enumeration of possible agent statuses.
		 *
		 * \since v.5.5.18
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
		 * \since v.5.5.18
		 */
		agent_status_t m_current_status;

		//! State listeners controller.
		impl::state_listener_controller_t m_state_listener_controller;

		/*!
		 * \brief Type of function for searching event handler.
		 *
		 * \since v.5.5.9
		 */
		using handler_finder_t =
			const impl::event_handler_data_t *(*)(
					execution_demand_t & /* demand */,
					const char * /* context_marker */ );

		/*!
		 * \brief Function for searching event handler.
		 *
		 * \note The value is set only once in the constructor and
		 * doesn't changed anymore.
		 *
		 * \since v.5.5.9
		 */
		handler_finder_t m_handler_finder;

		/*!
		 * \brief All agent's subscriptions.
		 *
		 * \since v.5.4.0
		 */
		impl::subscription_storage_unique_ptr_t m_subscriptions;

		/*!
		 * \brief Holder of message sinks for that agent.
		 *
		 * If message limits are defined for the agent it will be an actual
		 * storage with separate sinks for every (message_type, message_limit).
		 *
		 * If message limits are not defined then it will be a special storage
		 * with just one message sink (that sink will be used for all subscriptions).
		 *
		 * \since v.5.8.0
		 */
		std::unique_ptr< impl::sinks_storage_t > m_message_sinks;

		//! SObjectizer Environment for which the agent is belong.
		environment_t & m_env;

		/*!
		 * \brief Event queue operation protector.
		 *
		 * Initially m_event_queue is NULL. It is changed to actual value
		 * in so_bind_to_dispatcher() method. And reset to nullptr again
		 * in shutdown_agent().
		 *
		 * nullptr in m_event_queue means that methods push_event() will throw
		 * away any new demand.
		 *
		 * It is necessary to provide guarantee that m_event_queue will be reset
		 * to nullptr in shutdown_agent() only if there is no working
		 * push_event() methods. To do that default_rw_spinlock_t is used. Method
		 * push_event() acquire it in read-mode and shutdown_agent() acquires it
		 * in write-mode. It means that shutdown_agent() cannot get access to
		 * m_event_queue until there is working push_event().
		 *
		 * \since v.5.5.8
		 */
		default_rw_spinlock_t m_event_queue_lock;

		/*!
		 * \brief A pointer to event_queue.
		 *
		 * After binding to the dispatcher is it pointed to the actual
		 * event queue.
		 *
		 * After shutdown it is set to nullptr.
		 *
		 * \attention Access to m_event_queue value must be done only
		 * under acquired m_event_queue_lock.
		 *
		 * \since v.5.5.8
		 */
		event_queue_t * m_event_queue;

		/*!
		 * \brief A direct mbox for the agent.
		 *
		 * \since v.5.4.0
		 */
		const mbox_t m_direct_mbox;

		/*!
		 * \brief Working thread id.
		 *
		 * Some actions like managing subscriptions and changing states
		 * are enabled only on working thread id.
		 *
		 * \since v.5.4.0
		 */
		so_5::current_thread_id_t m_working_thread_id;

		//! Agent is belong to this cooperation.
		coop_t * m_agent_coop;

		/*!
		 * \brief Delivery filters for that agents.
		 *
		 * \note Storage is created only when necessary.
		 *
		 * \since v.5.5.5
		 */
		std::unique_ptr< impl::delivery_filter_storage_t > m_delivery_filters;

		/*!
		 * \brief Priority of the agent.
		 *
		 * \since v.5.5.8
		 */
		const priority_t m_priority;

		/*!
		 * \brief Binder for this agent.
		 *
		 * Since v.5.7.5 disp_binder for the agent is stored inside the agent.
		 * It guarantees that disp_binder will be deleted after destruction
		 * of the agent (if there is no circular references between the agent
		 * and the disp_binder).
		 *
		 * This value will be set by coop_t when agent is being add to the
		 * coop.
		 *
		 * \note
		 * Access to that field provided by so_5::impl::internal_agent_iface_t.
		 *
		 * \since v.5.7.5
		 */
		disp_binder_shptr_t m_disp_binder;

		/*!
		 * \brief Optional name for the agent.
		 *
		 * This value can be set in the constructor only and can't be changed
		 * later.
		 *
		 * Empty value means that the name for the agent wasn't specified.
		 *
		 * \since v.5.8.2
		 */
		const name_for_agent_t m_name;

		//! Destroy all agent's subscriptions.
		/*!
		 * \note
		 * This method is intended to be used in the destructor and
		 * methods like so_deactivate_agent().
		 *
		 * \attention
		 * It's noexcept method because there is no way to recover in case
		 * when deletion of subscriptions throws.
		 *
		 * \since v.5.7.3
		 */
		void
		destroy_all_subscriptions_and_filters() noexcept;

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
		 */
		void
		bind_to_coop(
			//! Cooperation for that agent.
			coop_t & coop );

		//! Agent shutdown deriver.
		/*!
		 * Method destroys all agent subscriptions.
		 *
		 * \since v.5.2.3
		 */
		void
		shutdown_agent() noexcept;
		/*!
		 * \}
		 */

		/*!
		 * \name Subscription/unsubscription implementation details.
		 * \{
		 */

		/*!
		 * \brief Helper function that returns a message sink to be used
		 * for subscriptions for specified message type.
		 *
		 * \since v.5.8.0
		 */
		[[nodiscard]]
		abstract_message_sink_t &
		detect_sink_for_message_type(
			const std::type_index & msg_type );

		/*!
		 * \brief Remove subscription for the state specified.
		 *
		 * \since v.5.2.3
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
		 * \brief Remove subscription for all states.
		 *
		 * \since v.5.2.3
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
		 * \since v.5.5.19.5
		 */
		bool
		do_check_subscription_presence(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type,
			//! State for the subscription.
			const state_t & target_state ) const noexcept;

		/*!
		 * \brief Check the presence of a deadletter handler.
		 *
		 * \since v.5.5.21
		 */
		bool
		do_check_deadletter_presence(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type ) const noexcept;
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
			const std::type_index & msg_type,
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
		 * \brief Calls so_evt_start method for agent.
		 *
		 * \since v.5.2.0
		 */
		static void
		demand_handler_on_start(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \brief Ensures that all agents from cooperation are
		 * bound to dispatchers.
		 *
		 * \since v.5.5.8
		 */
		void
		ensure_binding_finished();

		/*!
		 * \note This method is necessary for GCC on Cygwin.
		 *
		 * \since v.5.4.0
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_start_ptr() noexcept;

		/*!
		 * \brief Calls so_evt_finish method for agent.
		 *
		 * \since v.5.2.0
		 */
		static void
		demand_handler_on_finish(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \note This method is necessary for GCC on Cygwin.
		 *
		 * \since v.5.4.0
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_finish_ptr() noexcept;

		/*!
		 * \brief Calls event handler for message.
		 *
		 * \since v.5.2.0
		 */
		static void
		demand_handler_on_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \note This method is necessary for GCC on Cygwin.
		 *
		 * \since v.5.4.0
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_message_ptr() noexcept;

		/*!
		 * \brief Handles the enveloped message.
		 *
		 * \since v.5.5.23
		 */
		static void
		demand_handler_on_enveloped_msg(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since v.5.5.24
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_enveloped_msg_ptr() noexcept;
		/*!
		 * \}
		 */

	private :
		/*!
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
		 *
		 * \attention
		 * Implementation notes: it's important that \a method is passed
		 * by value. It's because subscription can be deleted during
		 * the work of process_message (due to unsubscription inside the
		 * event handler) and if pass \a method by a reference then
		 * that reference can become invalid.
		 *
		 * \since v.5.4.0
		 */
		static void
		process_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			thread_safety_t thread_safety,
			event_handler_method_t method );

		/*!
		 * \brief Actual implementation of enveloped message handling.
		 *
		 * \note
		 * handler_data can be nullptr. It means that an event handler
		 * for that message type if not found and special hook will
		 * be called for the envelope.
		 *
		 * \since v.5.5.23
		 */
		static void
		process_enveloped_msg(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			const impl::event_handler_data_t * handler_data );

		/*!
		 * \brief Enables operation only if it is performed on agent's
		 * working thread.
		 *
		 * \since v.5.4.0
		 */
		void
		ensure_operation_is_on_working_thread(
			const char * operation_name ) const;

		/*!
		 * \brief Drops all delivery filters.
		 *
		 * \since v.5.5.0
		 */
		void
		drop_all_delivery_filters() noexcept;

		/*!
		 * \brief Set a delivery filter.
		 *
		 * \since v.5.5.5
		 */
		void
		do_set_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			delivery_filter_unique_ptr_t filter );

		/*!
		 * \brief Drop a delivery filter.
		 *
		 * \since v.5.5.5
		 */
		void
		do_drop_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type ) noexcept;

		/*!
		 * \brief Handler finder for the case when message delivery
		 * tracing is disabled.
		 *
		 * \since v.5.5.9
		 */
		static const impl::event_handler_data_t *
		handler_finder_msg_tracing_disabled(
			execution_demand_t & demand,
			const char * context_marker );

		/*!
		 * \brief Handler finder for the case when message delivery
		 * tracing is enabled.
		 *
		 * \since v.5.5.9
		 */
		static const impl::event_handler_data_t *
		handler_finder_msg_tracing_enabled(
			execution_demand_t & demand,
			const char * context_marker );

		/*!
		 * \brief Actual search for event handler with respect
		 * to parent-child relationship between agent states.
		 *
		 * \since v.5.5.15
		 */
		static const impl::event_handler_data_t *
		find_event_handler_for_current_state(
			execution_demand_t & demand );

		/*!
		 * \brief Search for event handler between deadletter handlers.
		 *
		 * \return nullptr if event handler is not found.
		 *
		 * \since v.5.5.21
		 */
		static const impl::event_handler_data_t *
		find_deadletter_handler(
			execution_demand_t & demand );

		/*!
		 * \brief Perform actual operations related to state switch.
		 *
		 * It throws if the agent in awaiting_deregistration_state and
		 * \a state_to_be_set isn't awaiting_deregistration_state.
		 *
		 * \note
		 * This method doesn't check the working context. It's assumed
		 * that this check has already been performed by caller.
		 *
		 * \since v.5.7.3
		 */
		void
		do_change_agent_state(
			//! New state to be set as the current state.
			const state_t & state_to_be_set );

		/*!
		 * \brief Actual action for switching agent state.
		 *
		 * \since v.5.5.15
		 */
		void
		do_state_switch(
			//! New state to be set as the current state.
			const state_t & state_to_be_set ) noexcept;

		/*!
		 * \brief Return agent to the default state.
		 *
		 * \note This method is called just before invocation of
		 * so_evt_finish() to return agent to the default state.
		 * This return will initiate invocation of on_exit handlers
		 * for all active states of the agent.
		 *
		 * \attention State switch is not performed if agent is already
		 * in default state or if it waits deregistration after unhandled
		 * exception.
		 *
		 * \since v.5.5.15
		 */
		void
		return_to_default_state_if_possible() noexcept;

		/*!
		 * \brief Is agent already deactivated.
		 *
		 * Deactivated agent is in awaiting_deregistration_state.
		 * This method checks that the current state of the agent
		 * is awaiting_deregistration_state.
		 *
		 * \attention
		 * This method isn't thread safe and should be used with care.
		 * A caller should guarantee that it's called from the right
		 * working thread.
		 *
		 * \since v.5.7.3
		 */
		bool
		is_agent_deactivated() const noexcept;
};

/*!
 * \brief Helper function template for the creation of smart pointer
 * to an agent.
 *
 * This function can be useful if a pointer to an agent should be passed
 * somewhere with the guarantee that this pointer will remain valid even
 * if the agent will be deregistered.
 *
 * This could be necessary, for example, if a pointer to an agent is
 * passed to some callback (like it is done in Asio):
 * \code
 * void my_agent::on_some_event(mhood_t<some_msg> cmd) {
 * 	connection_.async_read_some(input_buffer_,
 * 		[self = so_5::make_agent_ref(this)](
 * 			const asio::error_code & ec,
 * 			std::size_t bytes_transferred )
 * 		{
 * 			if(!ec)
 * 				self->handle_incoming_data(bytes_transferred);
 * 		}
 * 	);
 * }
 * \endcode
 *
 * \since v.5.7.1
 */
template< typename Derived >
[[nodiscard]]
intrusive_ptr_t< Derived >
make_agent_ref( Derived * agent )
	{
		static_assert( std::is_base_of_v< agent_t, Derived >,
				"type should be derived from so_5::agent_t" );

		return { agent };
	}

template< typename Lambda >
void
agent_t::so_set_delivery_filter(
	const mbox_t & mbox,
	Lambda && lambda )
	{
		using namespace so_5::details::lambda_traits;

		using lambda_type = std::remove_reference_t<Lambda >;
		using argument_type =
				typename argument_type_if_lambda< lambda_type >::type;

		ensure_not_signal< argument_type >();

		do_set_delivery_filter(
				mbox,
				message_payload_type< argument_type >::subscription_type_index(),
				delivery_filter_unique_ptr_t{
					new low_level_api::lambda_as_filter_t< lambda_type, argument_type >(
							std::move( lambda ) )
				} );
	}

template< typename Lambda >
void
agent_t::so_set_delivery_filter_for_mutable_msg(
	const mbox_t & mbox,
	Lambda && lambda )
	{
		using namespace so_5::details::lambda_traits;

		using lambda_type = std::remove_reference_t<Lambda >;
		using argument_type =
				typename argument_type_if_lambda< lambda_type >::type;

		ensure_not_signal< argument_type >();

		using subscription_type = so_5::mutable_msg< argument_type >;

		do_set_delivery_filter(
				mbox,
				message_payload_type< subscription_type >::subscription_type_index(),
				delivery_filter_unique_ptr_t{
					new low_level_api::lambda_as_filter_t< lambda_type, argument_type >(
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
		details::is_agent_method_pointer<
				details::method_arity::unary,
				Method_Pointer>::value,
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
			thread_safety,
			event_handler_kind_t::final_handler );

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
			thread_safety,
			event_handler_kind_t::final_handler );

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

	//NOTE: shared_ptr is used because capture of unique_ptr
	//makes std::function non-copyable, but we need to copy
	//resulting 'method' object.
	//
	auto op_state = std::make_shared< transfer_op_state_t >(
			m_agent, m_mbox_ref->id(), outliving_const(target_state) );

	auto method = [op_state]( message_ref_t & msg )
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
					// We have very simple choice here: message is an enveloped
					// message or just classical message/signal.
					// So we should select an appropriate demand handler.
					message_kind_t::enveloped_msg == message_kind( msg ) ?
							agent_t::get_demand_handler_on_enveloped_msg_ptr() :
							agent_t::get_demand_handler_on_message_ptr()
			};

			demand.call_handler( query_current_thread_id() );
		};

	create_subscription_for_states(
			typeid( Msg ),
			method,
			thread_safety_t::unsafe,
			event_handler_kind_t::intermediate_handler );

	return *this;
}

template< typename Msg >
subscription_bind_t &
subscription_bind_t::suppress()
{
	// A method with nothing inside.
	auto method = []( message_ref_t & ) {};

	create_subscription_for_states(
			typeid( Msg ),
			method,
			thread_safety_t::safe,
			// Suppression of a message is a kind of ignoring of the message.
			// In the case of enveloped message intermediate_handler receives
			// the whole envelope (not the payload) and the whole envelope
			// should be ignored. We can't specify final_handler here because
			// in that case the payload of an enveloped message will be
			// extracted from the envelope and envelope will be informed about
			// the handling of message. But message won't be handled, it will
			// be ignored.
			event_handler_kind_t::intermediate_handler );

	return *this;
}

template< typename Msg >
subscription_bind_t &
subscription_bind_t::just_switch_to(
	const state_t & target_state )
{
	agent_t * agent_ptr = m_agent;

	auto method = [agent_ptr, &target_state]( message_ref_t & )
		{
			agent_ptr->so_change_state( target_state );
		};

	create_subscription_for_states(
			typeid( Msg ),
			method,
			thread_safety_t::unsafe,
			// Switching to some state is a kind of message processing.
			// So if there is an enveloped message then the envelope will be
			// informed about the processing of the payload.
			event_handler_kind_t::final_handler );

	return *this;
}

inline void
subscription_bind_t::create_subscription_for_states(
	const std::type_index & msg_type,
	const event_handler_method_t & method,
	thread_safety_t thread_safety,
	event_handler_kind_t handler_kind ) const
{
	if( m_states.empty() )
		// Agent should be subscribed only in default state.
		m_agent->so_create_event_subscription(
			m_mbox_ref,
			msg_type,
			m_agent->so_default_state(),
			method,
			thread_safety,
			handler_kind );
	else
		for( auto s : m_states )
			m_agent->so_create_event_subscription(
					m_mbox_ref,
					msg_type,
					*s,
					method,
					thread_safety,
					handler_kind );
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
state_t::is_active() const noexcept
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
		details::is_agent_method_pointer<
				details::method_arity::nullary,
				Method_Pointer>::value,
		state_t & >::type
state_t::on_enter( Method_Pointer pfn )
{
	using namespace details::event_subscription_helpers;

	using pfn_traits = details::is_agent_method_pointer<
			details::method_arity::nullary, Method_Pointer>;

	// Agent must have right type.
	auto cast_result =
			get_actual_agent_pointer<
					typename pfn_traits::agent_type >(
			*m_target_agent );

	return this->on_enter( [cast_result, pfn]() { (cast_result->*pfn)(); } );
}

template< typename Method_Pointer >
typename std::enable_if<
		details::is_agent_method_pointer<
				details::method_arity::nullary,
				Method_Pointer>::value,
		state_t & >::type
state_t::on_exit( Method_Pointer pfn )
{
	using namespace details::event_subscription_helpers;

	using pfn_traits = details::is_agent_method_pointer<
			details::method_arity::nullary, Method_Pointer>;

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

/*!
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
 *
 * \since v.5.5.1
 */
inline void
operator>>=( agent_t * agent, const state_t & new_state )
{
	agent->so_change_state( new_state );
}

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

