/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Testing related stuff.
 *
 * \since
 * v.5.5.24
 */

#pragma once

#include <so_5/all.hpp>

#include <so_5/h/stdcpp.hpp>

#include <sstream>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5 {
	
namespace experimental {
	
namespace testing {
	
SO_5_INLINE_NS namespace v1 {

namespace details {

//
// Forward declarations
//
class abstract_scenario_step_t;
class abstract_scenario_t;
class scenario_in_progress_accessor_t;

/*!
 * \brief A description of an event for testing scenario.
 *
 * Instances of that type will be passed to various hooks of
 * testing scenario and scenario's steps.
 *
 * \since
 * v.5.5.24
 */
struct incident_info_t final
	{
		//! Target of an event.
		const agent_t * m_agent;
		//! Type of message or signal.
		const std::type_index m_msg_type;
		//! ID of mbox from that message/signal was received.
		mbox_id_t m_src_mbox_id;

// Note. These constructors are necessary because VC++12 doesn't
// support initializators for structures in curly braces.
		incident_info_t() = default;
		incident_info_t(
			const agent_t * agent,
			const std::type_index & msg_type,
			mbox_id_t src_mbox_id )
			:	m_agent( agent )
			,	m_msg_type( msg_type )
			,	m_src_mbox_id( src_mbox_id )
			{}
	};

/*!
 * \brief What happened with source of an event.
 *
 * When a message or signal is delivered to an agent that message/signal
 * can be either handled or ignored. Some scenario triggers are activated
 * when source message/signal is handled, some are activated when incident
 * is ignored. This enumeration can be used for selection of that cases.
 *
 * \since
 * v.5.5.24
 */
enum class incident_status_t
	{
		//! Message or signal has been handled.
		handled,
		//! Message or signal has been ignored.
		ignored
	};

/*!
 * \brief Description of context on that a trigger is completed.
 *
 * Some triggers should do some actions on completion. Because
 * of that triggers should have access to scenario's step or even to
 * the whole scenario. This type holds a reference to objects those
 * can be accessible to a trigger.
 *
 * \note
 * The references inside that object are valid only for small amount
 * of time. They shouldn't be used after completion of trigger.
 *
 * \since
 * v.5.5.24
 */
struct trigger_completion_context_t
	{
		const scenario_in_progress_accessor_t & m_scenario_accessor;
		abstract_scenario_step_t & m_step;
	};

/*!
 * \brief An implementation of trigger for scenario's step.
 *
 * In the current version trigger is implemented as a concrete class (just for
 * simplicity), but in the future versions it could be an abstract interface.
 *
 * \since
 * v.5.5.24
 */
class SO_5_TYPE trigger_t final
	{
	public :
		using completion_function_t = std::function<
				void(const trigger_completion_context_t &) /*SO_5_NOEXCEPT*/ >;

	private :
		//! What should happen with initial message/signal.
		const incident_status_t m_incident_status;
		//! A reference to the target agent.
		/*!
		 * Note that this reference should be used with care.
		 * In complex scenarios an agent can be deregistered and
		 * this reference can point to free memory or reallocated
		 * for another agent memory.
		 *
		 * Before access to that reference it is necessary to check
		 * m_target_id field.
		 */
		const agent_t & m_target_agent;
		//! The unique ID or target's direct mbox.
		/*!
		 * ID of mbox is a unique value. ID is not reused even if
		 * the agent is destroyed and its memory is reallocated for
		 * another agent.
		 */
		const mbox_id_t m_target_id;
		//! Type of message/signal to activate the trigger.
		const std::type_index m_msg_type;
		//! ID of source mbox of message/signal to activate the trigger.
		const mbox_id_t m_src_mbox_id;

		//! Optional function for completion of the trigger.
		/*!
		 * If m_completion is empty then there is no need to
		 * separate completion action for the trigger and trigger
		 * become completed just after activation.
		 */
		completion_function_t m_completion;

		// Note. These are required by Clang.
		trigger_t( const trigger_t & ) = delete;
		trigger_t( trigger_t && ) = delete;

	public :
		//! Initializing constructor.
		trigger_t(
			incident_status_t incident_status,
			const agent_t & target,
			std::type_index msg_type,
			mbox_id_t src_mbox_id );
		~trigger_t();

		//! Get the reference of the target agent.
		/*!
		 * \attention
		 * This method should be used with care because if target agent
		 * is deregistered then a dangling reference will be returned.
		 */
		SO_5_NODISCARD
		const agent_t &
		target_agent() const SO_5_NOEXCEPT;

		//! Setter for completion function.
		void
		set_completion( completion_function_t fn );

		//! Check for activation of the trigger.
		/*!
		 * \retval true Trigger is activated.
		 * \retval false Trigger is not activated and information about
		 * the event can be used for checking for other triggers.
		 */
		SO_5_NODISCARD
		bool
		check(
			//! What happened with message/signal?
			const incident_status_t incident_status,
			//! Context for that event.
			const incident_info_t & info ) const SO_5_NOEXCEPT;

		//! Does this trigger require separate completion action?
		SO_5_NODISCARD
		bool
		requires_completion() const SO_5_NOEXCEPT;

		//! Do completion of a trigger.
		void
		complete(
			const trigger_completion_context_t & context ) SO_5_NOEXCEPT;
	};

/*!
 * \brief An alias for unique_ptr of trigger.
 *
 * \since
 * v.5.5.24
 */
using trigger_unique_ptr_t = std::unique_ptr< trigger_t >;

/*!
 * \brief An alias for type of tigger's container.
 *
 * \since
 * v.5.5.24
 */
using trigger_container_t = std::vector< trigger_unique_ptr_t >;

/*!
 * \brief A special data class with partial info for a new trigger.
 *
 * This data class contains a type of message/signal and optional
 * mbox_id for source mbox. If mbox_id is not specified then the direct
 * mbox of a target agent will be used as source mbox.
 *
 * \since
 * v.5.5.24
 */
template< incident_status_t Status >
struct trigger_source_t final
	{
		std::type_index m_msg_type;
		optional<mbox_id_t> m_src_mbox_id;

		trigger_source_t(
			std::type_index msg_type,
			mbox_id_t src_mbox_id)
			:	m_msg_type( std::move(msg_type) )
			,	m_src_mbox_id( src_mbox_id )
		{}

		trigger_source_t(
			std::type_index msg_type )
			:	m_msg_type( std::move(msg_type) )
		{}
	};

/*!
 * \brief A special data object for case of store-state-name completion action.
 *
 * \since
 * v.5.5.24
 */
struct store_agent_state_name_t
	{
		//! Name of tag for store-state-name action.
		std::string m_tag;
	};

/*!
 * \brief An interface of step's constraints.
 *
 * \since
 * v.5.5.24
 */
class constraint_t
	{
	public :
		constraint_t() = default;
		virtual ~constraint_t() SO_5_NOEXCEPT = default;

		constraint_t( const constraint_t & ) = delete;
		constraint_t & operator=( const constraint_t & ) = delete;

		constraint_t( constraint_t && ) = delete;
		constraint_t & operator=( constraint_t && ) = delete;

		//! Hook for step preactivation.
		/*!
		 * This hook will be called when step is preactivated.
		 * Constraint object can do some initial actions here. For
		 * example the current timestamp can be store. Or some resources
		 * can be acquired.
		 */
		virtual void
		start() SO_5_NOEXCEPT = 0;

		//! Hook for step completion.
		/*!
		 * This hook will be called when step is completed.
		 * Constraint object can do some cleanup actions here. For example
		 * resources acquired in start() method can be released.
		 */
		virtual void
		finish() SO_5_NOEXCEPT = 0;

		//! Check for fulfillment of constraint.
		/*!
		 * \retval true If constraint fulfilled.
		 * \retval false If constraint is not fulfilled and an incident
		 * should be ignored.
		 */
		SO_5_NODISCARD
		virtual bool
		check(
			//! What happened with message/signal?
			const incident_status_t incident_status,
			//! Context for that event.
			const incident_info_t & info ) const SO_5_NOEXCEPT = 0;
	};

/*!
 * \brief An alias for unique_ptr of constraint.
 *
 * \since
 * v.5.5.24
 */
using constraint_unique_ptr_t = std::unique_ptr< constraint_t >;

/*!
 * \brief An alias for container of constraints.
 *
 * \since
 * v.5.5.24
 */
using constraint_container_t = std::vector< constraint_unique_ptr_t >;

/*!
 * \brief Implementation of 'not_before' constraint.
 *
 * \since
 * v.5.5.24
 */
class not_before_constraint_t final
	:	public constraint_t
	{
		//! Value to be used.
		const std::chrono::steady_clock::duration m_pause;

		//! Time point of step preactivation.
		/*!
		 * Receives value only in start() method.
		 */
		std::chrono::steady_clock::time_point m_started_at;

	public :
		not_before_constraint_t(
			std::chrono::steady_clock::duration pause )
			:	m_pause( pause )
			{}

		void
		start() SO_5_NOEXCEPT override
			{
				m_started_at = std::chrono::steady_clock::now();
			}

		void
		finish() SO_5_NOEXCEPT override { /* nothing to do */ }

		SO_5_NODISCARD
		bool
		check(
			const incident_status_t /*incident_status*/,
			const incident_info_t & /*info*/ ) const SO_5_NOEXCEPT override
			{
				return m_started_at + m_pause <=
						std::chrono::steady_clock::now();
			}
	};

/*!
 * \brief Implementation of 'not_after' constraint.
 *
 * \since
 * v.5.5.24
 */
class not_after_constraint_t final
	:	public constraint_t
	{
		//! Value to be used.
		const std::chrono::steady_clock::duration m_pause;

		//! Time point of step preactivation.
		/*!
		 * Receives value only in start() method.
		 */
		std::chrono::steady_clock::time_point m_started_at;

	public :
		not_after_constraint_t(
			std::chrono::steady_clock::duration pause )
			:	m_pause( pause )
			{}

		void
		start() SO_5_NOEXCEPT override
			{
				m_started_at = std::chrono::steady_clock::now();
			}

		void
		finish() SO_5_NOEXCEPT override { /* nothing to do */ }

		SO_5_NODISCARD
		bool
		check(
			const incident_status_t /*incident_status*/,
			const incident_info_t & /*info*/ ) const SO_5_NOEXCEPT override
			{
				return m_started_at + m_pause >
						std::chrono::steady_clock::now();
			}
	};

/*!
 * \brief An alias for type of step's preactivation action.
 *
 * \since
 * v.5.5.24
 */
using preactivate_action_t = std::function< void() /*SO_5_NOEXCEPT*/ >;

/*!
 * \brief An interface of testing scenario step.
 *
 * This interface is described in a public header file just for
 * definition of step_definition_proxy_t class. But abstract_scenario_step_t
 * is an internal and implementation-specific type, please don't use
 * it in end-user code.
 *
 * \since
 * v.5.5.24
 */
class SO_5_TYPE abstract_scenario_step_t
	{
	public :
		//! Status of step.
		enum class status_t
			{
				//! Step is not preactivated yet.
				//! Step can be changed while it in this state.
				passive,
				//! Step is preactivated.
				//! It means that this step is the current step in testing
				//! scenario now. But not all required triggers are activated
				//! yet.
				preactivated,
				//! Step is activated.
				//! It means that all required triggers are activated, but
				//! some triggers wait for completion actions.
				active,
				//! Step is completed.
				//! It means that all required triggers were activated and
				//! all of them were completed.
				completed
			};

		/*!
		 * \brief Type of token returned from pre-handler-hook.
		 *
		 * This token can be in valid or invalid states. If is is in
		 * valid state then it should be passed as is to
		 * abstract_scenario_step_t::post_handler_hook() method.
		 *
		 * \since
		 * v.5.5.24
		 */
		class token_t final
			{
				//! Activated trigger.
				/*!
				 * Can be null. It means that there was no activated triggers
				 * at pre_handler_hook() method. And token is in invalid state.
				 *
				 * This pointer can also be null if trigger was activated but
				 * wasn't require completion. It means that trigger switched
				 * to completed state and there is no need in separate
				 * completion action.
				 *
				 * If this pointer is not null then post_handler_hook()
				 * should be called for scenario step.
				 */
				trigger_t * m_trigger;

			public:
				token_t() SO_5_NOEXCEPT
					:	m_trigger( nullptr )
					{}
				token_t( trigger_t * trigger ) SO_5_NOEXCEPT
					:	m_trigger( trigger )
					{}

				SO_5_NODISCARD
				bool
				valid() const SO_5_NOEXCEPT { return m_trigger != nullptr; }

				//! Get a reference to activated trigger.
				/*!
				 * This method should be called only if token is in valid
				 * state.
				 */
				SO_5_NODISCARD
				trigger_t &
				trigger() const SO_5_NOEXCEPT { return *m_trigger; }
			};

		// Note. These are required by Clang compiler.
		abstract_scenario_step_t() = default;
		abstract_scenario_step_t( const abstract_scenario_step_t & ) = delete;
		abstract_scenario_step_t & operator=( const abstract_scenario_step_t & ) = delete;
		abstract_scenario_step_t( abstract_scenario_step_t && ) = delete;
		abstract_scenario_step_t & operator=( abstract_scenario_step_t && ) = delete;

		virtual ~abstract_scenario_step_t() = default;

		//! Get the name of the step.
		SO_5_NODISCARD
		virtual const std::string &
		name() const SO_5_NOEXCEPT = 0;

		//! Perform preactivation of the step.
		/*!
		 * Preactivation means that the step becomes the current step
		 * of the scenario and all events will go to
		 * pre_handler_hook(), post_handler_hook() and
		 * no_handler_hook() of that step.
		 *
		 * This method should call all preactivation actions passed to
		 * the step via add_peactivate_action() method.
		 */
		virtual void
		preactivate() SO_5_NOEXCEPT = 0;

		//! Hook that should be called before invocation of event-handler.
		/*!
		 * This hook should be called before invocation of any event-handler
		 * for ordinary message, service request or enveloped message.
		 *
		 * The step should update its status inside that method.
		 *
		 * If a valid token is returned then this token should be passed
		 * to subsequent call to post_handler_hook() method.
		 */
		SO_5_NODISCARD
		virtual token_t
		pre_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT = 0;

		//! Hook that should be called just after completion of event-handler.
		/*!
		 * If previous call to pre_handler_hook() returned a valid
		 * token object then this hook should be called just after
		 * completion of event-handler.
		 */
		virtual void
		post_handler_hook(
			const scenario_in_progress_accessor_t & scenario_accessor,
			token_t token ) SO_5_NOEXCEPT = 0;

		//! Hook that should be called if there is no event-handler for
		//! a message or service request.
		/*!
		 * The step should update its status inside that method.
		 */
		virtual void
		no_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT = 0;

		//! Get the current status of the step.
		SO_5_NODISCARD
		virtual status_t
		status() const SO_5_NOEXCEPT = 0;

		//! Add another preactivation action.
		/*!
		 * Can be called several times. New action will be added to the end
		 * of list of preactivation actions.
		 */
		virtual void
		add_preactivate_action(
			preactivate_action_t action ) = 0;

		//! Setup triggers for the step.
		/*!
		 * It is assumed that this method will be called only once per step.
		 * If it is called several times the new information will replace
		 * the old one.
		 *
		 * \note
		 * Value of \a triggers_to_activate has sence for case of
		 * step_definition_proxy_t::when_any() method: there can be
		 * several triggers in \a triggers container, but only one of
		 * them has to be triggered to activate the step.
		 */
		virtual void
		setup_triggers(
			trigger_container_t triggers,
			std::size_t triggers_to_activate ) SO_5_NOEXCEPT = 0;

		//! Setup constraints for the step.
		/*!
		 * It is assumed that this method will be called only once per
		 * step. If it is called several times the new information will
		 * replace the old one.
		 */
		virtual void
		setup_constraints(
			constraint_container_t constraints ) SO_5_NOEXCEPT = 0;
	};

/*!
 * \brief An alias for unique_ptr of scenario-step.
 *
 * \since
 * v.5.5.24
 */
using step_unique_ptr_t = std::unique_ptr< abstract_scenario_step_t >;

/*!
 * \brief A helper class for holding unique_ptr to a trigger while
 * trigger is being configured.
 *
 * \note
 * This class is Movable, but not Copyable.
 *
 * \since
 * v.5.5.24
 */
template< incident_status_t Status >
class trigger_holder_t final
	{
		trigger_unique_ptr_t m_trigger;

	public :
		trigger_holder_t( trigger_unique_ptr_t trigger ) SO_5_NOEXCEPT
			:	m_trigger( std::move(trigger) )
			{}

#if defined(SO_5_MSVC_CANT_DEFAULT_MOVE_CONSTRUCTOR)
		trigger_holder_t( trigger_holder_t && o ) SO_5_NOEXCEPT
			:	m_trigger( std::move(o.m_trigger) )
			{}
		trigger_holder_t & operator=( trigger_holder_t && o ) SO_5_NOEXCEPT
			{
				m_trigger = std::move(o.m_trigger);
				return *this;
			}
#else
		trigger_holder_t( trigger_holder_t && ) SO_5_NOEXCEPT = default;
		trigger_holder_t & operator=( trigger_holder_t && ) SO_5_NOEXCEPT = default;
#endif

		trigger_holder_t( const trigger_holder_t & ) = delete;
		trigger_holder_t & operator=( const trigger_holder_t & ) = delete;

		//! Get the trigger object from the holder.
		/*!
		 * \note
		 * Holder becomes empty and should not be used anymore
		 * (except for assigning a new value);
		 */
		trigger_unique_ptr_t
		giveout_trigger() SO_5_NOEXCEPT
			{
				return std::move(m_trigger);
			}
	};

} /* namespace details */

/*!
 * \brief A special object that should be used for definition of a step
 * of a testing scenario.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * testing_env_t env;
 * 
 * so_5::agent_t * test_agent = ...;
 * so_5::agent_t * another_agent = ...;
 *
 * auto scenario = env.scenario();
 *
 * // Create a simple step. It triggers when agent receives a message
 * // from its direct mbox.
 * scenario.define_step("one").when(*test_agent & reacts_to<some_message>());
 *
 * // Create a step with constraint: a message should be received only
 * // after a 15ms from preactivation of this step.
 * scenario.define_step("two")
 * 	.constraints(not_before(std::chrono::milliseconds(15)))
 * 	.when(*test_agent & reacts_to<another_message>(some_mbox));
 *
 * // Create a step with initial actions: several messages will be sent
 * // during preactivation of this step.
 * // There are also two constraints: a time range for receiving expected
 * // message.
 * scenario.define_step("three")
 * 	.impact<first_message>(first_target)
 * 	.impact<second_message>(second_target, arg1, arg2, arg3)
 * 	.constraints(
 * 		not_before(std::chrono::milliseconds(15)),
 * 		not_after(std::chrono::seconds(2)))
 * 	.when(*test_agent & reacts_to<expected_message>(some_mbox));
 *
 * // Create a step that trigges when both agents receive messages.
 * scenario.define_step("four")
 * 	.when_all(
 * 		*test_agent & reacts_to<one_message>(some_mbox),
 * 		*another_agent & reacts_to<different_message>(another_mbox));
 *
 * // Create a step that triggers when one of agents receive a message.
 * scenario.define_step("five")
 * 	.when_any(
 * 		*test_agent & reacts_to<one_message>(some_mbox),
 * 		*another_agent & reacts_to<different_message>(another_mbox));
 * \endcode
 *
 * \note
 * The object of this type can be stored to define a step in several
 * footsteps:
 * \code
 * auto step = env.scenario().define_step("my_step");
 * if(some_condition)
 * 	step.constraints(...);
 * if(another_condition)
 * 	step.impact(...);
 * if(third_condition)
 * 	step.when(...);
 * else
 * 	step.when_all(...);
 * \endcode
 * But all definitions should be done before calling of
 * scenario_proxy_t::run_for().
 *
 * \attention
 * This class is not thread-safe. Because of that it should be used on
 * a context of a signle thread.
 *
 * \since
 * v.5.5.24
 */
class step_definition_proxy_t
	{
		details::abstract_scenario_step_t * m_step;

		void
		append_trigger_to( details::trigger_container_t & /*to*/ ) {}

		template<
			details::incident_status_t Status,
			typename... Args >
		void
		append_trigger_to(
			details::trigger_container_t & to,
			details::trigger_holder_t<Status> event,
			Args && ...args )
			{
				to.emplace_back( event.giveout_trigger() );
				append_trigger_to( to, std::forward<Args>(args)... );
			}

		void
		append_constraint_to( details::constraint_container_t & /*to*/ ) {}

		template< typename... Args >
		void
		append_constraint_to(
			details::constraint_container_t & to,
			details::constraint_unique_ptr_t head,
			Args && ...tail )
			{
				to.emplace_back( std::move(head) );
				append_constraint_to( to, std::forward<Args>(tail)... );
			}

	public :
		//! Initializing constructor.
		/*!
		 * Despite the fact that this is a public constructor
		 * it is a part of SObjectizer implementation and is subject
		 * of change without a notice. Don't use it in end-user code.
		 */
		step_definition_proxy_t(
			details::abstract_scenario_step_t * step )
			:	m_step( step )
			{}

		/*!
		 * \brief Define a preactivation action in form of
		 * sending a message/signal to the specified target.
		 *
		 * This method creates and stores an instance of a message/signal
		 * and then sends this instance when step is preactivated.
		 *
		 * Usage example:
		 * \code
		 * env.scenario().define_step("my_step")
		 * 	.impact<my_message>(*test_agent, arg1, arg2, arg3)
		 * 	.impact<my_signal>(some_mbox)
		 * 	.impact<another_message>(mchain)
		 * 	...;
		 * \endcode
		 *
		 * Please note that this method can be called several times.
		 *
		 * \tparam Msg_Type type of a message/signal to send.
		 * \tparam Target type of a target for message/signal.
		 * It can be a reference to mbox, a reference to agent
		 * (agent's direct mbox will be used in that case), or
		 * a reference to mchain.
		 * \tparam Args types of arguments for Msg_Type's constructor.
		 */
		template<
			typename Msg_Type,
			typename Target,
			typename... Args >
		step_definition_proxy_t &
		impact( Target && target, Args && ...args )
			{
				// Deduce actual mbox of the recevier.
				// This mbox will be captured by lambda function.
				auto to = so_5::send_functions_details::arg_to_mbox(
						std::forward<Target>(target) );

				// Make an instance of a message.
				// This instance will be captured by lambda function.
				message_ref_t msg{
					so_5::details::make_message_instance<Msg_Type>(
							std::forward<Args>(args)... )
				};
				// Mutability of a message should be changed appropriately.
				change_message_mutability(
						msg,
						message_payload_type< Msg_Type >::mutability() );

				// Now we can create a lambda-function that will send
				// the message instance at the appropriate time.
				m_step->add_preactivate_action(
						[to, msg]() SO_5_NOEXCEPT {
							to->deliver_message(
									message_payload_type< Msg_Type >
											::subscription_type_index(),
									msg );
						} );

				return *this;
			}

//FIXME: there should be check that Lambda is a callable object.
//NOTE: resolution of this FIXME is postponed until support of
//MSVS2013 will be cancelled.

		/*!
		 * \brief Add preactivation action in form of lambda-object.
		 *
		 * This method can be used for non-trivial actions on step
		 * preactivation, like sending enveloped messages.
		 *
		 * Usage example:
		 * \code
		 * env.scenario().define_step("my_step")
		 * 	.impact([some_mbox, some_data] {
		 * 		some_mbox->do_deliver_enveloped_msg(
		 * 			so_5::message_payload_type<my_message>::subscription_type_index(),
		 *  			std::make_unique<my_envelope<my_message>>(some_data),
		 *  			1u);
		 * 	});
		 * \endcode
		 *
		 * Please note that this method can be called several times.
		 *
		 * \note
		 * Preactivation of scenario step is performed when scenario object
		 * is locked. Becase of that actions inside \a lambda must be performed
		 * with care to avoid deadlocks or blocking of scenario for too long.
		 *
		 * \attention
		 * \a lambda should be noexcept-function.
		 */
		template< typename Lambda >
		step_definition_proxy_t &
		impact( Lambda && lambda )
			{
				m_step->add_preactivate_action(
						[lambda]() SO_5_NOEXCEPT { lambda(); } );

				return *this;
			}

		/*!
		 * \brief Add a tigger for activation of that step
		 *
		 * Step is activated when this trigger is activated.
		 *
		 * Usage example:
		 * \code
		 * env.scenario().define_step("my_step")
		 * 	.when(some_agent & reacts_to<my_message>());
		 * \endcode
		 *
		 * \note
		 * This method is indended to be called only once.
		 * All subsequent calls to that method will replace triggers
		 * those where set by previous calls to when(), when_all() and
		 * when_any() methods.
		 */
		template< details::incident_status_t Status >
		step_definition_proxy_t &
		when(
			details::trigger_holder_t<Status> event )
			{
				details::trigger_container_t cnt;
				cnt.emplace_back( event.giveout_trigger() );

				m_step->setup_triggers( std::move(cnt), 1u );

				return *this;
			}

		/*!
		 * \brief Add a list of tiggers for activation of that step
		 *
		 * Step is activated when \b any of those triggers is activated.
		 *
		 * Usage example:
		 * \code
		 * env.scenario().define_step("my_step")
		 * 	.when_any(
		 * 		some_agent & reacts_to<my_message>(),
		 * 		another_agent & reacts_to<another_message>(),
		 * 		yet_another_agent & reacts_to<yet_another_message>());
		 * \endcode
		 *
		 * \note
		 * This method is indended to be called only once.
		 * All subsequent calls to that method will replace triggers
		 * those where set by previous calls to when(), when_all() and
		 * when_any() methods.
		 */
		template<
			details::incident_status_t Status,
			typename... Args >
		step_definition_proxy_t &
		when_any(
			details::trigger_holder_t<Status> event,
			Args && ...args )
			{
				details::trigger_container_t cnt;
				cnt.reserve( 1u + sizeof...(args) );

				append_trigger_to(
						cnt,
						std::move(event),
						std::forward<Args>(args)... );

				m_step->setup_triggers( std::move(cnt), 1u );

				return *this;
			}

		/*!
		 * \brief Add a list of tiggers for activation of that step
		 *
		 * Step is activated when \a all of those triggers is activated.
		 *
		 * Usage example:
		 * \code
		 * env.scenario().define_step("my_step")
		 * 	.when_all(
		 * 		some_agent & reacts_to<my_message>(),
		 * 		another_agent & reacts_to<another_message>(),
		 * 		yet_another_agent & reacts_to<yet_another_message>());
		 * \endcode
		 *
		 * \note
		 * This method is indended to be called only once.
		 * All subsequent calls to that method will replace triggers
		 * those where set by previous calls to when(), when_all() and
		 * when_any() methods.
		 */
		template<
			details::incident_status_t Status,
			typename... Args >
		step_definition_proxy_t &
		when_all(
			details::trigger_holder_t<Status> event,
			Args && ...args )
			{
				const auto total_triggers = 1u + sizeof...(args);

				details::trigger_container_t cnt;
				cnt.reserve( total_triggers );

				append_trigger_to(
						cnt,
						std::move(event),
						std::forward<Args>(args)... );

				m_step->setup_triggers( std::move(cnt), total_triggers );

				return *this;
			}

		/*!
		 * \brief Add a list of constraints for that step
		 *
		 * All specified constraints should be fulfilled to enable
		 * activation of that step.
		 *
		 * Usage example:
		 * \code
		 * env.scenario().define_step("my_step")
		 * 	.constraints(
		 * 		not_before(std::chrono::milliseconds(10)))
		 * 	...;
		 * env.scenario().define_step("another_step")
		 * 	.constraints(
		 * 		not_after(std::chrono::milliseconds(500),
		 * 		not_before(std::chrono::milliseconds(10)))
		 * 	...;
		 * \endcode
		 *
		 * \note
		 * This method is indended to be called only once.
		 * All subsequent calls to that method will replace constraints
		 * those where set by previous calls to constraints().
		 */
		template< typename... Args >
		step_definition_proxy_t &
		constraints(
			details::constraint_unique_ptr_t head,
			Args && ...tail )
			{
				details::constraint_container_t cnt;
				cnt.reserve( 1u + sizeof...(tail) );

				append_constraint_to(
						cnt,
						std::move(head),
						std::forward<Args>(tail)... );

				m_step->setup_constraints( std::move(cnt) );

				return *this;
			}
	};

/*!
 * \brief Status of testing scenario.
 *
 * This enumeration is used by testing scenario itself and by
 * scenario_result_t type.
 *
 * \since
 * v.5.5.24
 */
enum class scenario_status_t
	{
		//! Testing scenario is not started yet.
		//! New step can be added when scenario is in state.
		not_started,
		//! Testing scenario is started but not finished yet.
		//! New steps can't be added.
		in_progress,
		//! Testing scenario is successfuly completed.
		completed,
		//! Testing scenario is not working any more, but it is not
		//! completed becase there is no more time to run the scenario.
		timed_out
	};

/*!
 * \brief The result of run of testing scenario.
 *
 * The result contains the status of the scenario
 * (in form of scenario_status_t enumeration) and optional textual
 * description of the result.
 *
 * Note that description is missing if testing scenario completed
 * successfuly (it means that scenario has scenario_status_t::completed
 * state after completion of scenario_proxy_t::run_for() method).
 *
 * The content and format of textual description is not specified and
 * can be changed in the future versions of SObjectizer.
 *
 * Usage example:
 * \code
 * TEST_CASE("some_case") {
 * 	using namespace so_5::experimental::testing;
 *
 * 	testing_env_t env;
 * 	...
 * 	env.scenario().run_for(std::chrono::milliseconds(500));
 *
 * 	REQUIRE(completed() == env.scenario().result());
 * }
 * \endcode
 *
 * \note
 * Object of scenario_result_t type can be dumped to std::ostream.
 * For example:
 * \code
 * auto result = env.scenario().result();
 * if(completed() != result)
 * 	std::cout << "The result is: " << result << std::endl;
 * \endcode
 *
 * \since
 * v.5.5.24
 */
class scenario_result_t
	{
		scenario_status_t m_status;
		optional< std::string > m_description;

	public :
		//! The constructor for a case when there is only status of scenario.
		scenario_result_t(
			scenario_status_t status )
			:	m_status( status )
			{}
		//! The constructor for a case when there are status and description
		//! of scenario.
		scenario_result_t(
			scenario_status_t status,
			std::string description )
			:	m_status( status )
			,	m_description( std::move(description) )
			{}

		//! Check for equality.
		/*!
		 * \note
		 * Only status is compared.
		 */
		SO_5_NODISCARD
		bool
		operator==( const scenario_result_t & o ) const SO_5_NOEXCEPT
			{
				return m_status == o.m_status;
			}

		//! Check for inequality.
		/*!
		 * \note
		 * Only status is compared.
		 */
		SO_5_NODISCARD
		bool
		operator!=( const scenario_result_t & o ) const SO_5_NOEXCEPT
			{
				return m_status != o.m_status;
			}

		//! Dump of object's content to ostream.
		friend std::ostream &
		operator<<( std::ostream & to, const scenario_result_t & v )
			{
				const auto status_name =
					[](scenario_status_t st) -> const char * {
						const char * result{};
						switch( st )
							{
							case scenario_status_t::not_started :
								result = "not_started";
							break;
							case scenario_status_t::in_progress :
								result = "in_progress";
							break;
							case scenario_status_t::completed :
								result = "completed";
							break;
							case scenario_status_t::timed_out :
								result = "timed_out";
							break;
							};
						return result;
					};

				to << "[" << status_name( v.m_status );
				if( v.m_description )
					to << ",{" << *v.m_description << "}";
				to << "]";

				return to;
			}
	};

/*!
 * \brief Create a value that means that scenario completed successfuly.
 *
 * Usage example:
 * \code
 * TEST_CASE("some_case") {
 * 	using namespace so_5::experimental::testing;
 *
 * 	testing_env_t env;
 * 	...
 * 	env.scenario().run_for(std::chrono::milliseconds(500));
 *
 * 	REQUIRE(completed() == env.scenario().result());
 * }
 * \endcode
 *
 * \since
 * v.5.5.24
 */
SO_5_NODISCARD
inline scenario_result_t
completed() { return { scenario_status_t::completed }; }

/*!
 * \brief Define a trigger that activates when an agent receives and
 * handles a message from the direct mbox.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * ...
 * env.scenario().define_step("my_step")
 * 	.when(some_agent & reacts_to<some_message>());
 * \endcode
 *
 * \since
 * v.5.5.24
 */
template<typename Msg_Type>
details::trigger_source_t< details::incident_status_t::handled >
reacts_to()
	{
		return { message_payload_type<Msg_Type>::subscription_type_index() };
	}

/*!
 * \brief Define a trigger that activates when an agent receives and
 * handles a message from the specific mbox.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * ...
 * env.scenario().define_step("my_step")
 * 	.when(some_agent & reacts_to<some_message>(some_mbox));
 * \endcode
 *
 * \since
 * v.5.5.24
 */
template<typename Msg_Type>
details::trigger_source_t< details::incident_status_t::handled >
reacts_to( const so_5::mbox_t & mbox )
	{
		return {
				message_payload_type<Msg_Type>::subscription_type_index(),
				mbox->id() 	
			};
	}

/*!
 * \brief Create a special marker for a trigger for storing
 * agent's state name inside scenario.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * ...
 * env.scenario().define_step("my_step")
 * 	.when(some_agent & reacts_to<some_message>() & store_agent_name("my_agent"));
 * ...
 * env.scenario().run_for(std::chrono::seconds(1));
 *
 * REQUIRE(completed() == env.scenario().result());
 * REQUIRE("some_state" == env.scenario().stored_state_name("my_step", "my_agent"));
 * \endcode
 *
 * \since
 * v.5.5.24
 */
inline details::store_agent_state_name_t
store_state_name( std::string tag )
	{
		return { std::move(tag) };
	}

/*!
 * \brief Define a trigger that activates when an agent rejects a message from
 * the direct mbox.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * ...
 * env.scenario().define_step("my_step")
 * 	.when(some_agent & ignores<some_message>());
 * \endcode
 *
 * \attention
 * It is necessary that agent should be subscribed to that message but
 * ignores it in the current state.
 * If an agent is not subscribed to a message then the message can be
 * simply skipped inside a call to send() function. It means that there
 * won't be delivery of message at all.
 *
 * \since
 * v.5.5.24
 */
template<typename Msg_Type>
details::trigger_source_t< details::incident_status_t::ignored >
ignores()
	{
		return { message_payload_type<Msg_Type>::subscription_type_index() };
	}

/*!
 * \brief Define a trigger that activates when an agent rejects a message from
 * the direct mbox.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * ...
 * env.scenario().define_step("my_step")
 * 	.when(some_agent & ignores<some_message>(some_mbox));
 * \endcode
 *
 * \attention
 * It is necessary that agent should be subscribed to that message but
 * ignores it in the current state.
 * If an agent is not subscribed to a message then the message can be
 * simply skipped inside a call to send() function. It means that there
 * won't be delivery of message at all.
 *
 * \since
 * v.5.5.24
 */
template<typename Msg_Type>
details::trigger_source_t< details::incident_status_t::ignored >
ignores( const so_5::mbox_t & mbox )
	{
		return {
				message_payload_type<Msg_Type>::subscription_type_index(),
				mbox->id() 	
			};
	}

/*!
 * \brief Create a constraint not-before.
 *
 * That constraint is fulfilled if an event is happened after
 * a specified \a pause. Time is calculated from moment of preactivation
 * of a scenario's step.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * env.scenario().define_step("my_step")
 * 	.constraints(not_before(std::chrono::milliseconds(50)))
 * 	.when(some_agent & reacts_to<some_message>());
 * \endcode
 * In that case step won't be activated if agent receives a message
 * after, for example, 15ms.
 *
 * \note
 * If not_before() is used with not_after() then the correctness of those
 * constraints is not checked.
 *
 * \since
 * v.5.5.24
 */
inline details::constraint_unique_ptr_t
not_before(
	std::chrono::steady_clock::duration pause )
	{
		return stdcpp::make_unique< details::not_before_constraint_t >(pause);
	}

/*!
 * \brief Create a constraint not-after.
 *
 * That constraint is fulfilled if an event is happened before
 * a specified \a pause. Time is calculated from moment of preactivation
 * of a scenario's step.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * env.scenario().define_step("my_step")
 * 	.constraints(not_after(std::chrono::milliseconds(50)))
 * 	.when(some_agent & reacts_to<some_message>());
 * \endcode
 * In that case step won't be activated if agent receives a message
 * after, for example, 55ms.
 *
 * \note
 * If not_after() is used with not_before() then the correctness of those
 * constraints is not checked.
 *
 * \since
 * v.5.5.24
 */
inline details::constraint_unique_ptr_t
not_after(
	std::chrono::steady_clock::duration pause )
	{
		return stdcpp::make_unique< details::not_after_constraint_t >(pause);
	}

namespace details {

/*!
 * \brief A special objects that allows to call some specific methods
 * of a testing scenario.
 *
 * In the current version abstract_scenario_t has at least one
 * method that should be called only when testing scenario is in
 * progess. This method requires an instance of that class as a special
 * token.
 *
 * \since
 * v.5.5.24
 */
class scenario_in_progress_accessor_t final
	{
		friend class abstract_scenario_t;

	private :
		outliving_reference_t<abstract_scenario_t> m_scenario;

		scenario_in_progress_accessor_t(
			outliving_reference_t<abstract_scenario_t> scenario )
			:	m_scenario( scenario )
			{}

		scenario_in_progress_accessor_t(
			const scenario_in_progress_accessor_t & ) = delete;
		scenario_in_progress_accessor_t & operator=(
			const scenario_in_progress_accessor_t & ) = delete;

		scenario_in_progress_accessor_t(
			scenario_in_progress_accessor_t && ) = delete;
		scenario_in_progress_accessor_t & operator=(
			scenario_in_progress_accessor_t && ) = delete;

	public :
		abstract_scenario_t &
		scenario() const SO_5_NOEXCEPT { return m_scenario.get(); }
	};

/*!
 * \brief An interface of testing scenario.
 *
 * Please note that this type is a part of SObjectizer implementation
 * and is subject of changes in upcoming version. Do not use it in your
 * code, there is scenario_proxy_t call intended to be used by
 * end-users.
 *
 * This class is described in a public header-file in the current
 * version of SObjectizer. This is just to simplify implementation of
 * some testing-related stuff. The definition of that class can be move
 * to another file in future versions of SObjectizer without any
 * previous notice.
 *
 * \since
 * v.5.5.24
 */
class abstract_scenario_t
	{
	protected :
		//! Helper method for creation of scenario_in_progress_accessor instance.
		SO_5_NODISCARD
		scenario_in_progress_accessor_t
		make_accessor() SO_5_NOEXCEPT
			{
				return { outliving_mutable(*this) };
			}

	public :
		/*!
		 * \brief Type of token returned by pre-event-handler hook.
		 *
		 * Object of that type should be stored and then it should
		 * be passed to abstract_scenario_t::post_handler_hook() method.
		 *
		 * A token can be valid. It means that token holds an actual
		 * pointer to some scenario step.
		 *
		 * Or token can be invalid. It means that there is no a valid
		 * pointer to scenario step. In that case methods like
		 * activated_step() and step_token() should not be called.
		 *
		 * \since v.5.5.24
		 */
		class token_t final
			{
				abstract_scenario_step_t * m_activated_step;
				abstract_scenario_step_t::token_t m_step_token;

			public :
				token_t()
					:	m_activated_step( nullptr )
					{}
				token_t(
					abstract_scenario_step_t * activated_step,
					abstract_scenario_step_t::token_t step_token )
					:	m_activated_step( activated_step )
					,	m_step_token( step_token )
					{}

				SO_5_NODISCARD
				bool
				valid() const SO_5_NOEXCEPT
					{
						return nullptr != m_activated_step;
					}

				SO_5_NODISCARD
				abstract_scenario_step_t &
				activated_step() const SO_5_NOEXCEPT
					{
						return *m_activated_step;
					}

				SO_5_NODISCARD
				abstract_scenario_step_t::token_t
				step_token() const SO_5_NOEXCEPT
					{
						return m_step_token;
					}
			};

	public :
		abstract_scenario_t() = default;

		abstract_scenario_t( const abstract_scenario_t & ) = delete;
		abstract_scenario_t & operator=( const abstract_scenario_t & ) = delete;

		abstract_scenario_t( abstract_scenario_t && ) = delete;
		abstract_scenario_t & operator=( abstract_scenario_t && ) = delete;

		virtual ~abstract_scenario_t() = default;

		//! Create a new step and return proxy for it.
		SO_5_NODISCARD
		virtual step_definition_proxy_t
		define_step( nonempty_name_t step_name ) = 0;

		//! Get the result of scenario execution.
		SO_5_NODISCARD
		virtual scenario_result_t
		result() const SO_5_NOEXCEPT = 0;

		//! Run the scenario until completion or for specific amount of time.
		virtual void
		run_for( std::chrono::steady_clock::duration run_time ) = 0;

		//! Hook that should be called before invocation of event-handler.
		/*!
		 * This hook should be called before invocation of any event-handler
		 * for ordinary message, service request or enveloped message.
		 *
		 * The token returned should then be passed to post_handler_hook().
		 */
		SO_5_NODISCARD
		virtual token_t
		pre_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT = 0;

		//! Hook that should be called just after completion of event-handler.
		/*!
		 * This hook should be called just after completion of any event-handler
		 * for ordinary message, service request or enveloped message.
		 */
		virtual void
		post_handler_hook(
			//! Token returned by previous pre_handler_hook() call.
			token_t token ) SO_5_NOEXCEPT = 0;

		//! Hook that should be called if there is no event-handler for
		//! a message or service request.
		virtual void
		no_handler_hook(
			const incident_info_t & info ) SO_5_NOEXCEPT = 0;

		//! Store a name of an agent state in the scenario.
		/*!
		 * Note this method can be accessed only when scenario object is locked.
		 */
		virtual void
		store_state_name(
			const scenario_in_progress_accessor_t & /*accessor*/,
			const abstract_scenario_step_t & step,
			const std::string & tag,
			const std::string & state_name ) = 0;

		//! Get the stored state name.
		/*!
		 * Should be called only after completion of scenario.
		 *
		 * Will throw an exception if there is no stored state for
		 * a pair of (\a step_name,\a tag).
		 */
		SO_5_NODISCARD
		virtual std::string
		stored_state_name(
			const std::string & step_name,
			const std::string & tag ) const = 0;
	};

/*!
 * \brief A helper operator to create a trigger for the specified agent.
 *
 * \since
 * v.5.5.24
 */
template< incident_status_t Status >
trigger_holder_t<Status>
operator&(
	const so_5::agent_t & agent,
	const trigger_source_t<Status> & src )
	{
		const auto src_mbox_id = src.m_src_mbox_id ?
				*(src.m_src_mbox_id) : agent.so_direct_mbox()->id();

		return {
				stdcpp::make_unique<trigger_t>(
						Status,
						agent,
						src.m_msg_type,
						src_mbox_id )
			};
	}

/*!
 * \brief A helper operator to create a tigger that stores the
 * name of the current agent's state.
 *
 * \since
 * v.5.5.24
 */
inline trigger_holder_t<incident_status_t::handled>
operator&(
	trigger_holder_t<incident_status_t::handled> && old_holder,
	store_agent_state_name_t data )
	{
		auto trigger_ptr = old_holder.giveout_trigger();
		auto * target_agent = &(trigger_ptr->target_agent());
//FIXME: for C++14 and newest standard there should be move of
//data content to the lambda object.
		trigger_ptr->set_completion(
				[data, target_agent](
					const trigger_completion_context_t & ctx ) SO_5_NOEXCEPT
				{
					ctx.m_scenario_accessor.scenario().store_state_name(
							ctx.m_scenario_accessor,
							ctx.m_step,
							data.m_tag,
							target_agent->so_current_state().query_name() );
				} );

		return { std::move(trigger_ptr) };
	}

} /* namespace details */

/*!
 * \brief A special wrapper around scenario object.
 *
 * The class scenario_proxy_t is a public interface to scenario object.
 * The actual scenario object is inside of testing_env_t instance and
 * access to it is provided via scenario_proxy_t wrapper.
 *
 * Usage example:
 * \code
 * using namespace so_5::experimental::testing;
 * TEST_CASE("some_case") {
 * 	testing_env_t env;
 *
 * 	so_5::agent_t * test_agent;
 * 	env.environment().introduce_coop([&](so_5::coop_t & coop) {
 * 		test_agent = coop.make_agent<some_agent>();
 * 	});
 *
 * 	env.scenario().define_step("one")
 * 		.impact<some_message>(*test_agent)
 * 		.when(*test_agent & reacts_to<some_message>());
 *
 * 	env.scenario().run_for(std::chrono::milliseconds(200));
 *
 * 	REQUIRE(completed() == env.scenario().result());
 * }
 * \endcode
 * Or in more conciseness form:
 * \code
 * using namespace so_5::experimental::testing;
 * TEST_CASE("some_case") {
 * 	testing_env_t env;
 *
 * 	so_5::agent_t * test_agent;
 * 	env.environment().introduce_coop([&](so_5::coop_t & coop) {
 * 		test_agent = coop.make_agent<some_agent>();
 * 	});
 *
 * 	auto scenario = env.scenario();
 *
 * 	scenario.define_step("one")
 * 		.impact<some_message>(*test_agent)
 * 		.when(*test_agent & reacts_to<some_message>());
 *
 * 	scenario.run_for(std::chrono::milliseconds(200));
 *
 * 	REQUIRE(completed() == scenario.result());
 * }
 * \endcode
 *
 * \note
 * scenario_proxy_t holds a reference to an object from testing_env_t
 * instance. It means that scenario_proxy shouldn't outlive the
 * corresponding testing_env_t object. For example this code will
 * lead to a dangling reference:
 * \code
 * so_5::experimental::testing::scenario_proxy_t get_scenario() {
 * 	so_5::experimental::testing::testing_env_t env;
 * 	return env.scenario(); // OOPS! A reference to destroyed object will be returned.
 * }
 * \endcode
 *
 * \since
 * v.5.5.24
 */
class SO_5_TYPE scenario_proxy_t final
	{
		friend class testing_env_t;

	private :
		outliving_reference_t< details::abstract_scenario_t > m_scenario;

		scenario_proxy_t(
			outliving_reference_t< details::abstract_scenario_t > scenario );

	public :
		//! Start definition of a new scenario's step.
		/*!
		 * New steps can be defined until run_for() is called.
		 * When the scenario is in progress (or is already
		 * finished) then an attempt to call to define_step() will
		 * lead to an exception.
		 *
		 * \return A special wrapper around a new step instance.
		 * This wrapped should be used to define the step.
		 *
		 * \note
		 * The value of \a step_name has to be unique, but the current
		 * version of SObjectizer doesn't controll that.
		 */
		SO_5_NODISCARD
		step_definition_proxy_t
		define_step( nonempty_name_t step_name );

		//! Get the result of scenario execution.
		/*!
		 * \note
		 * This method is intended to be called only after the completion of
		 * run_for() method.
		 */
		SO_5_NODISCARD
		scenario_result_t
		result() const;

		//! Runs the scenario for specified amount of time.
		/*!
		 * This method does to things:
		 * - unfreeze all agents registered in testing environment
		 *   up to this time;
		 * - run scenario until scenario will be completed or
		 *   \a run_time elapsed.
		 *
		 * The result of scenario run can then be obtained by result()
		 * method.
		 *
		 * Usage example:
		 * \code
		 * using namespace so_5::experimental::testing;
		 * TEST_CASE("some_case") {
		 * 	testing_env_t env; // Create a testing environment.
		 *
		 *    // Introduce some agents.
		 *    env.environment().introduce_coop(...);
		 *
		 * 	// Do define the scenario for the test case.
		 * 	auto scenario = env.scenario();
		 * 	...
		 * 	// Run the scenario for at most 1 second.
		 * 	scenario.run_for(std::chrono::seconds(1));
		 * 	// Check the result of the scenario.
		 * 	REQUIRE(completed() == scenario.result());
		 * }
		 * \endcode
		 */
		void
		run_for( std::chrono::steady_clock::duration run_time );

		//! Try to get stored name of an agent's state.
		/*!
		 * This method allows to get state name stored by
		 * store_state_name() trigger. For example:
		 * \code
		 * using namespace so_5::experimental::testing;
		 * TEST_CASE("some_case") {
		 * 	testing_env_t env;
		 *
		 * 	so_5::agent_t * test_agent;
		 * 	env.environment().introduce_coop([&](so_5::coop_t & coop) {
		 * 		test_agent = coop.make_agent<some_agent_type>(...);
		 * 	});
		 *
		 * 	env.scenario().define_step("one")
		 * 		.impact<some_message>(*test_agent, ...)
		 * 		.when(*test_agent & reacts_to<some_message>()
		 * 				& store_state_name("my_agent"));
		 * 	...
		 * 	env.scenario().run_for(std::chrono::seconds(1));
		 *
		 * 	REQUIRE(completed() == env.scenario().result());
		 *
		 * 	REQUIRE("some_state" == env.scenario().stored_state_name("one", "my_agent"));
		 * }
		 * \endcode
		 *
		 * \attention
		 * This method can be called only after completion of the scenario.
		 * Otherwise an instance of so_5::exception_t will be thrown.
		 *
		 * \return Name of stored state. If there is no stored name for
		 * a pair of (\a step_name, \a tag) then an instance of
		 * so_5::exception_t will be thrown.
		 */
		SO_5_NODISCARD
		std::string
		stored_state_name(
			const std::string & step_name,
			const std::string & tag ) const;
	};

/*!
 * \brief A special testing environment that should be used for
 * testing of agents.
 *
 * To allow testing of agent a special environment is necessary.
 * That environment creates special hooks those control event
 * handling. The class testing_env_t is an implementation of that
 * special environment.
 *
 * To create a test-case for some agent (or even several agents)
 * is it necessary to create an instance of testing_env_t class:
 * \code
 * using namespace so_5::experimental::testing;
 * TEST_CASE("some_case") {
 * 	testing_env_t env;
 * 	...
 * }
 * \endcode
 * An instance of testing_env_t creates and launches SObjectizer
 * Environment in the constructor. That Environment will be shut down
 * in the destructor of testing_env_t instance automatically.
 * It is possible to shut down SObjectizer Environment manually
 * by using stop(), join(), stop_then_join() methods
 * (they play the same role as the corresponding methods of
 * so_5::wrapped_env_t class).
 *
 * The testing_env_t instance already has an instance of scenario
 * object inside. Access to that object can be obtained via
 * scenario() method:
 * \code
 * using namespace so_5::experimental::testing;
 * TEST_CASE("some_case") {
 * 	testing_env_t env;
 * 	...
 * 	env.scenario().define_step("one")...;
 * 	...
 * 	// Or, for conciseness:
 * 	auto scenario = env.scenario();
 * 	scenario.define_step("two")...;
 * 	...
 * }
 * \endcode
 *
 * Please note that this is a special environment. One important
 * feature of it is a behaviour of agents created inside that
 * environment. All agents registered before call to
 * scenario_proxy_t::run_for() method will be in "frozen" mode:
 * they are present in the SObjectizer Environment, but they can't
 * receive any incoming messages (even so_5::agent_t::so_evt_start()
 * is not called for them).
 *
 * All registered agents will be unfreezed when scenario_proxy_t::run_for()
 * will be called (or if testing_env_t is stopped). For example:
 * \code
 * class hello final : public so_5::agent_t {
 * public:
 * 	using so_5::agent_t::agent_t;
 * 	void so_evt_start() override {
 * 		std::cout << "Hello, World!" << std::endl;
 * 	}
 * };
 * void testing_env_demo() {
 * 	so_5::experimental::testing::testing_env_t env;
 *
 * 	env.environment().introduce_coop([](so_5::coop_t & coop) {
 * 		coop.make_agent<hello>();
 * 	});
 *
 * 	std::cout << "Bye, bye!" << std::endl;
 *
 * 	env.scenario().run_for(std::chrono::milliseconds(100));
 * }
 * \endcode
 * In that case we will have the following output:
\verbatim
Bye, bye!
Hello, World!
\endverbatim
 *
 * \since
 * v.5.5.24
 */
class SO_5_TYPE testing_env_t
	{
	public :
		//! Default constructor.
		/*!
		 * \note
		 * This constructor launches SObjectizer Environment with
		 * default parameters.
		 */
		testing_env_t();
		//! A constructor that allows to tune environment's parameters.
		/*!
		 * Usage example:
		 * \code
		 * using namespace so_5::experimental::testing;
		 * testing_env_t env{
		 * 	[](so_5::environment_params_t & params) {
		 * 		// Turn message delivery tracing on.
		 * 		params.message_delivery_tracer(
		 * 			so_5::msg_tracing::std_cout_tracer());
		 * 	}
		 * };
		 * \endcode
		 *
		 * \note
		 * The testing_env_t class can change some values in environment_params_t
		 * after the return from \a env_params_tuner.
		 */
		testing_env_t(
			//! Function for environment's params tuning.
			so_5::api::generic_simple_so_env_params_tuner_t env_params_tuner );
		//! A constructor that receives already constructed
		//! environment parameters.
		/*!
		 * Usage example:
		 * \code
		 * so_5::environment_params_t make_params() {
		 * 	so_5::environment_params_t params;
		 * 	...
		 * 	params.message_delivery_tracer(
		 * 		so_5::msg_tracing::std_cout_tracer());
		 * 	...
		 * 	return params;
		 * }
		 * ...
		 * TEST_CASE("some_case") {
		 * 	using namespace so_5::experimental::testing;
		 * 	testing_env_t env{ make_params() };
		 * 	...
		 * }
		 * \endcode
		 *
		 * \note
		 * The testing_env_t class can change some values in environment_params_t
		 * before launching a SObjectizer Environment.
		 */
		testing_env_t(
			environment_params_t && env_params );
		~testing_env_t();

		//! Access to wrapped environment.
		environment_t &
		environment() const;

		//! Send stop signal to environment.
		/*!
		 * Please note that stop() just sends a shut down signal to the
		 * SObjectizer Environment, but doesn't wait the completion of
		 * the Environment. If you calls stop() method you have to
		 * call join() to ensure that Environment is shut down.
		 */
		void
		stop();

		//! Wait for complete finish of environment's work.
		void
		join();

		//! Send stop signal and wait for complete finish of environment's work.
		void
		stop_then_join();

		//! Access to the associated scenario.
		SO_5_NODISCARD
		scenario_proxy_t
		scenario() SO_5_NOEXCEPT;

		struct internals_t;

	private :
		std::unique_ptr< internals_t > m_internals;

		wrapped_env_t m_sobjectizer;

		void
		tune_environment_on_start( environment_t & env );

		void
		wait_init_completion();
	};

} /* namespace v1 */

} /* namespace testing */

} /* namespace experimental */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

