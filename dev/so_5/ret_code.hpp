/*
	SObjectizer 5
*/

/*!
	\file
	\brief Error codes.
*/

#pragma once

#include <so_5/declspec.hpp>

#include <iostream>

namespace so_5
{

//
// Standard error codes.
//

//! so_environment launch is failed.
const int rc_environment_error = 1;

//! \name Error codes for agent's methods.
//! \{

//! Trying to switch to the unknown state.
const int rc_agent_unknown_state = 10;

//! Agent is not bound to a cooperation.
const int rc_agent_has_no_cooperation = 14;

//! It is impossible to make a cast to that type.
const int rc_agent_incompatible_type_conversion = 15;

/*!
 * \since
 * v.5.4.0
 *
 * \brief An attempt to perform an operation which is enabled only
 * on agent's working thread.
 */
const int rc_operation_enabled_only_on_agent_working_thread = 16;

/*!
 * \since
 * v.5.5.15
 *
 * \brief An attempt to change agent state to a new composite state which
 * have no initial state defined.
 */
const int rc_no_initial_substate = 17;

/*!
 * \since
 * v.5.5.15
 *
 * \brief Nesting of agent states is too deep.
 */
const int rc_state_nesting_is_too_deep = 18;

/*!
 * \since
 * v.5.5.15
 *
 * \brief Initial substate for a composite state is already defined.
 */
const int rc_initial_substate_already_defined = 19;

//! \}

//! \name Error codes for cooperations of agents.
//! \{

//! Cooperation couldn't be registered.
const int rc_coop_define_agent_failed = 24;

/*!
 * \since
 * v.5.4.0
 *
 * \brief Binding of agent to dispatcher failed.
 */
const int rc_agent_to_disp_binding_failed = 27;

/*!
 * \since
 * v.5.4.0.1
 *
 * \brief It is impossible to register cooperation during
 * SObjectizer Environment shutdown.
 */
const int rc_unable_to_register_coop_during_shutdown = 28;
//! \}


//! \name Error codes for dispatchers.
//! \{

//! Unable to create a dispatcher.
const int rc_disp_create_failed = 31;

//! \}

//! \name Error codes for event handlers and message interceptors registration.
//! \{

//! A handler for that event/mbox/state is already registered.
const int rc_evt_handler_already_provided = 40;

//! Agent doesn't own this state.
const int rc_agent_is_not_the_state_owner = 43;

/*!
 * \since
 * v.5.4.0
 *
 * \brief An attempt to create illegal subscription to mpsc_mbox.
 */
const int rc_illegal_subscriber_for_mpsc_mbox = 47;

/*!
 * \since
 * v.5.5.4
 *
 * \brief An attempt to create subscription to message without
 * predefined limit for that message type.
 */
const int rc_message_has_no_limit_defined = 48;

/*!
 * \since
 * v.5.5.4
 *
 * \brief An attempt to define several limits for one message type.
 */
const int rc_several_limits_for_one_message_type = 49;

//! \}

//! \name Error codes for mboxes.
//! \{

//! Null message data.
/*!
 * \since
 * v.5.2.0
 *
 *
 * An attempt to deliver message_t by nullptr pointer to it is an error.
 * A signal should be used for this.
 */
const int rc_null_message_data = 82;

//! \}

//! \name Error codes for layers.
//! \{

//! The layer is not bound to the SObjectizer Environment.
const int rc_layer_not_binded_to_so_env = 100;

//! Unable to bind a layer by the null pointer to it.
const int rc_trying_to_add_nullptr_extra_layer = 101;

//! The layer is already bound to the SObjectizer Environment as a default layer.
const int rc_trying_to_add_extra_layer_that_already_exists_in_default_list = 102;

//! The layer is already bound to the SObjectizer Environment as an extra layer.
const int rc_trying_to_add_extra_layer_that_already_exists_in_extra_list = 103;

//! Layer initialization is failed.
const int rc_unable_to_start_extra_layer = 104;

//! A layer with the specified type doesn't exist.
const int rc_layer_does_not_exist = 105;
//! \}

//! \name Error codes for priorities.
//! \{

//! Illegal value of quote for a priority.
const int rc_priority_quote_illegal_value = 120;

//! \}

//! \name Error codes for SObjectizer Environment related errors.
//! \{

//! Message delivery tracing is disabled and cannot be used.
const int rc_msg_tracing_disabled = 140;

//! \}

//! \name Error codes for message chains.
//! \{

//! Attempt to get message from empty message queue.
const int rc_msg_chain_is_empty = 160;

//! Attempt to push a message to full message queue.
const int rc_msg_chain_is_full = 161;

//! Attempt to make subscription for message chain.
const int rc_msg_chain_doesnt_support_subscriptions = 162;

//! Attempt to set delivery_filter for message chain.
const int rc_msg_chain_doesnt_support_delivery_filters = 163;

//! Attempt to added a message to full message chain when
//! throw_exception overflow reaction is used.
const int rc_msg_chain_overflow = 164;

//! Attempt to define several handlers for one msg_type.
const int rc_several_handlers_for_one_message_type = 165;

//! \}

/*!
 * \brief An attempt to switch agent state when another switch operation
 * is in progress.
 *
 * \since
 * v.5.5.18
 */
const int rc_another_state_switch_in_progress = 170;

/*!
 * \brief An attempt to launch environment with autoshutdown disabled in
 * conditions where autoshutdown must be enabled.
 *
 * For example so_5::env_infrastructures::simple_not_mtsafe requires
 * autoshutdown feature.
 *
 * \since
 * v.5.5.19
 */
const int rc_autoshutdown_must_be_enabled = 171;

/*!
 * \brief An attempt to deliver mutable message via MPMC mbox.
 *
 * \since
 * v.5.5.19
 */
const int rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox = 172;

/*!
 * \brief An attempt to send mutable message as a periodic message
 *
 * \since
 * v.5.5.19
 */
const int rc_mutable_msg_cannot_be_periodic = 173;

/*!
 * \brief An attempt to make subscription on mutable message from
 * MPMC mbox.
 *
 * Delivery of mutable messages via MPMC mboxes is prohibited.
 *
 * \since
 * v.5.5.19
 */
const int rc_subscription_to_mutable_msg_from_mpmc_mbox = 174;

/*!
 * \brief An attempt to set up a new stop_guard when the stop operation
 * is already in progress.
 *
 * \since
 * v.5.5.19.2
 */
const int rc_cannot_set_stop_guard_when_stop_is_started = 175;

/*!
 * \brief An attempt to use negative value for pause argument for
 * delayed or periodic message/signal.
 *
 * A value of `pause` argument for so_5::send_delayed() and
 * so_5::send_periodic() must be non-negative.
 *
 * \since
 * v.5.5.21
 */
const int rc_negative_value_for_pause = 176;

/*!
 * \brief An attempt to use negative value for period argument for
 * periodic message/signal.
 *
 * A value of `pause` argument for so_5::send_periodic() must be non-negative.
 *
 * \since
 * v.5.5.21
 */
const int rc_negative_value_for_period = 177;

/*!
 * \brief A loop in transfer_to_state detected.
 *
 * There could be a case when usage of transfer_to_state leads to a loop.
 * For a very simple example:
 * \code
 * class a_two_state_loop_t final : public so_5::agent_t
 * {
 * 	state_t st_one{ this, "one" };
 * 	state_t st_two{ this, "two" };
 * 
 * 	struct message {};
 * 
 * public :
 * 	a_two_state_loop_t(context_t ctx) : so_5::agent_t{ctx} {
 * 		this >>= st_one;
 * 
 * 		st_one.transfer_to_state<message>(st_two);
 * 		st_two.transfer_to_state<message>(st_one);
 * 	}
 * 
 * 	virtual void so_evt_start() override {
 * 		so_5::send<message>(*this);
 * 	}
 * };
 * \endcode
 * Since v.5.5.22.1 an attempt to handle `message` in the example above
 * will lead to an exception with this error code.
 *
 * Note that there could be more tricky examples when hierarchical states
 * are used:
 * \code
 * class a_hsm_case_t final : public so_5::agent_t
 * {
 * 	state_t st_base{ this, "base" };
 * 	state_t st_disconnected{ initial_substate_of{st_base}, "disconnected" };
 * 	state_t st_connected{ substate_of{st_base}, "connected" };
 * 
 * 	struct message {};
 * 
 * public :
 * 	a_hsm_case_t(context_t ctx) : so_5::agent_t{ctx} {
 * 		this >>= st_base;
 * 
 * 		st_base.transfer_to_state<message>(st_disconnected);
 * 	}
 * 
 * 	virtual void so_evt_start() override {
 * 		so_5::send<message>(*this);
 * 	}
 * };
 * \endcode
 *
 * \since
 * v.5.5.22.1
 */
const int rc_transfer_to_state_loop = 178;

/*!
 * \brief An attempt to cast message to message envelope when
 * a pointer to message is NULL.
 *
 * Envelopes were introduced in v.5.5.23. Envelopes are sent and
 * delivered as ordinary messages (it means that pointer to envelope
 * will be sent as pointer to message_t). For processing of an envelope
 * it is necessary to cast a pointer from message_t to envelope_t.
 * But it is impossible if a pointer to message_t is null.
 *
 * \since
 * v.5.5.23
 */
const int rc_attempt_to_cast_to_envelope_on_nullptr = 180;

/*!
 * \brief New step can't be defined if testing scenario is already
 * started or finished.
 *
 * \since
 * v.5.5.24
 */
const int rc_unable_to_define_new_step = 181;

/*!
 * \brief Testing scenario must be completed before an attempt
 * to do the current operation.
 *
 * \since
 * v.5.5.24
 */
const int rc_scenario_must_be_completed = 182;

/*!
 * \brief There is no stored state name in the testing scenario.
 *
 * \since
 * v.5.5.24
 */
const int rc_stored_state_name_not_found = 183;

/*!
 * \brief An attempt to get a pointer to already destroyed cooperation.
 *
 * This error can be reported when a coop_handle instance for already
 * destroyed coop is used.
 *
 * \since
 * v.5.6.0
 */
const int rc_coop_already_destroyed = 184;

/*!
 * \brief An attempt to do something with coop that is not in registered
 * state.
 *
 * For example if can be an attempt to add a new child coop when
 * the parent coop is being deregistered.
 *
 * \since
 * v.5.6.0
 */
const int rc_coop_is_not_in_registered_state = 185;

/*!
 * \brief An attempt to call join() from the joinable thread itself.
 *
 * \since
 * v.5.6.0
 */
const int rc_unable_to_join_thread_by_itself = 186;

/*!
 * \brief An attempt to modify or activate extensible-select when an operation
 * on that extensible-select object is already active.
 *
 * \since
 * v.5.6.1
 */
const int rc_extensible_select_is_active_now = 187;

/*!
 * \brief An attempt to activate prepared-select when an operation
 * on that prepared-select object is already active.
 *
 * \since
 * v.5.6.1
 */
const int rc_prepared_select_is_active_now = 188;

/*!
 * \brief Agent can't change state because the agent is already deactivated.
 *
 * There is a special state for an agent in that agent can't handle events and
 * only waits for the deregistration. An agent is switched to that state
 * due to unhandled exception (with exception reaction set to
 * so_5::deregister_coop_on_exception) or after by a call to
 * agent_t::so_deactivate_agent(). It is impossible to change the state of
 * the agent if the agent is in that state already.
 *
 * \since v.5.7.3
 */
const int rc_agent_deactivated = 189;

/*!
 * \brief An instance of MPSC mbox is expected as custom direct mbox.
 *
 * Version 5.7.4 allows to set a custom mbox as the direct mbox for
 * an agent. That mbox has to be MPSC mbox. An attempt to set MPMC
 * mbox as the direct mbox will lead to this error.
 *
 * \since v.5.7.4
 */
const int rc_mpsc_mbox_expected = 190;

/*!
 * \brief nullptr can't be passed as delivery_filter.
 *
 * An attempt to pass nullptr pointer to a delivery_filter (or empty
 * delivery_filter_unique_ptr_t) to a method/function that expects
 * an actual instance of delivery_filter.
 *
 * \since v.5.8.0
 */
const int rc_nullptr_as_delivery_filter_pointer = 191;

/*!
 * \brief nullptr returned by user-provided mbox factory.
 *
 * The so_5::environment_t::introduce_named_mbox() gets a user-provided
 * mbox factory. This factory is called by SObjectizer Environment
 * to create a new instance of the named mbox. This factory must return
 * a valid so_5::mbox_t. If the returned so_5::mbox_t contains
 * nullptr it's an error and this error code will be used in an
 * exception thrown by so_5::environment_t::introduce_named_mbox().
 *
 * \since v.5.8.0
 */
const int rc_nullptr_as_result_of_user_mbox_factory = 192;

/*!
 * \brief There are no resources that must have been in place for an agent in
 * advance.
 *
 * Some dispatchers (like so_5::disp::nef_one_thread) preallocates
 * resources necessary for normal agent work. If those resource are not
 * found for an agent for some reason it's an error and exception with
 * this error code is thrown.
 *
 * \since v.5.8.0
 */
const int rc_no_preallocated_resources_for_agent = 193;

/*!
 * \brief The dispatcher binder is already set for the agent.
 *
 * Since v.5.7.5 disp_binder is stored inside the agent.
 * Only one disp_binder can be set for an agent. An attempt to set
 * disp_binder when the agent already has one is an error.
 *
 * \since v.5.7.5
 */
const int rc_disp_binder_already_set_for_agent = 194;

/*!
 * \brief The dispatcher binder is not set for the agent yet.
 *
 * Since v.5.7.5 disp_binder is stored inside the agent.
 * A disp_binder must be set for an agent during addition of the agent
 * to a coop. It means that if agent is added to coop the agent should have
 * non-null pointer to the disp_binder. If this pointer is still null then
 * it's an error.
 *
 * \since v.5.7.5
 */
const int rc_no_disp_binder_for_agent = 195;

/*!
 * \brief Name for an agent can't be empty.
 *
 * \since v.5.8.2
 */
const int rc_empty_agent_name = 196;

/*!
 * \brief Length of an agent name is too large.
 *
 * \since v.5.8.2
 */
const int rc_agent_name_too_long = 197;

//! \name Common error codes.
//! \{

//! The empty name doesn't allowed.
const int rc_empty_name = 500;

/*!
 * \brief Invalid value of time limit for an agent's state.
 *
 * \since v.5.5.15
 */
const int rc_invalid_time_limit_for_state = 501;

/*!
 * \brief Feature or method has no implementation yet.
 *
 * \since v.5.5.16
 */
const int rc_not_implemented = 502;

/*!
 * \brief An exception of unknown type is caught.
 *
 * SObjectizer expects that all exceptions are derived from std::exception
 * class.
 *
 * \since v.5.5.24.3
 */
const int rc_unknown_exception_type = 503;

//! Unclassified error.
const int rc_unexpected_error = 0xFFFFFF;
//! \}

} /* namespace so_5 */

