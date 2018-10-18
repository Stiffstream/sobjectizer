/*
	SObjectizer 5
*/

/*!
	\file
	\brief Error codes.
*/

#pragma once

#include <so_5/h/declspec.hpp>

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

//! Agent is already bound to the SObjectizer Environment.
const int rc_agent_is_already_bind_to_env = 11;

//! Agent is already bound to the dispatcher.
const int rc_agent_is_already_bind_to_disp = 12;

//! Dispatcher with that name is not found.
const int rc_named_disp_not_found = 13;

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

//! A zero pointer is detected.
const int rc_zero_ptr_to_coop = 20;

//! Cooperation name is not unique.
const int rc_coop_with_specified_name_is_already_registered = 21;

//! Cooperation has the null reference to an agent or dispatcher binding.
const int rc_coop_has_references_to_null_agents_or_binders = 22;

//! There is no registered cooperation with that name.
const int rc_coop_has_not_found_among_registered_coop = 23;

//! Cooperation couldn't be registered.
const int rc_coop_define_agent_failed = 24;

/*!
 * \since
 * v.5.2.3
 *
 * \brief Cooperation has no the parent cooperation.
 */
const int rc_coop_has_no_parent = 25;

/*!
 * \since
 * v.5.2.3
 *
 * \brief Parent cooperation not found.
 */
const int rc_parent_coop_not_found = 26;

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

//! Unable to start a dispatcher.
const int rc_disp_start_failed = 30;

//! Unable to create a dispatcher.
const int rc_disp_create_failed = 31;

//! The expected type of dispatcher and the actual type of the 
//! agent dispatcher are not the same.
const int rc_disp_type_mismatch = 32;

/*!
 * \since
 * v.5.4.0
 *
 * \brief New dispatcher cannot be added because of illegal
 * state of SObjectizer Environment.
 */
const int rc_disp_cannot_be_added = 34;

//! \}

//! \name Error codes for event handlers and message interceptors registration.
//! \{

//! A handler for that event/mbox/state is already registered.
const int rc_evt_handler_already_provided = 40;

//! A handler for that event/mbox/state is not registered and cannot be
//! deregistered.
const int rc_no_event_handler_provided = 41;

//! Unable to deregister that handler.
/*!
 * There is a handler for the event/mbox/state, but it is another handler.
 * Not the one is specified for the deregistration.
 */
const int rc_event_handler_match_error = 42;

//! Agent doesn't own this state.
const int rc_agent_is_not_the_state_owner = 43;

//! An interceptor for the event/mbox/state is already registered.
const int rc_intercept_handler_already_provided = 44;

//! An interceptor for the event/mbox/state is not registered and
//! cannot be deregistered.
const int rc_no_interception_handler_provided = 45;

//! Unable to deregister this interceptor.
/*!
 * There is an interceptor for the event/mbox/state, but it is another handler.
 * Not the one is specified for the deregistration.
 */
const int rc_interception_handler_match_error = 46;

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

//! The mbox name is not unique.
const int rc_mbox_duplicating_name = 80;

//! The name of mbox is unknown.
const int rc_mbox_unable_to_find_mbox = 81;

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

/*!
 * \since
 * v.5.3.0
 *
 * \brief No handlers for service request found.
 */
const int rc_no_svc_handlers = 83;

/*!
 * \since
 * v.5.3.0
 *
 * \brief There are more then one handler for service request.
 */
const int rc_more_than_one_svc_handler = 84;

/*!
 * \since
 * v.5.3.0
 *
 * \brief Service request has not been handled. Service handler 
 * is disabled in the current agent state.
 */
const int rc_svc_not_handled = 85;

/*!
 * \since
 * v.5.3.0
 *
 * \brief Service request result has not beed received yet.
 */
const int rc_svc_result_not_received_yet = 86;

/*!
 * \since
 * v.5.3.0
 *
 * \brief Unable to cast service call description object to
 * the appropriate type.
 *
 * This error is possible when type of service request doesn't match
 * actual type of request handler. For example:
 *
 *
\code
// Actual service type.
int a_some_svc_t::evt_query_state( mhood_t< msg_query_state > )
	{ ... }

// Service request call with wrong return type.
mbox.get_one< std::string >().wait_forever().sync_get<msg_query_state>();
\endcode
 */
const int rc_msg_service_request_bad_cast = 87;

/*!
 * \since
 * v.5.5.4.
 *
 * \brief Unable to transform service request as part of overlimit reaction.
 */
const int rc_svc_request_cannot_be_transfomred_on_overlimit = 88;

/*!
 * \since
 * v.5.5.5
 *
 * \brief Delivery filter is not applicable to MPSC-mboxes.
 */
const int rc_delivery_filter_cannot_be_used_on_mpsc_mbox = 89;

//! \}

//! \name Error codes for delayed or repeated events.
//! \{

//! Unable to schedule a timer event.
const int rc_unable_to_schedule_timer_act = 90;
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
 * so_5::send_periodic() (also for so_5::environment_t::schedule_timer()
 * and so_5::environment_t::single_timer()) must be non-negative.
 *
 * \since
 * v.5.5.21
 */
const int rc_negative_value_for_pause = 176;

/*!
 * \brief An attempt to use negative value for period argument for
 * periodic message/signal.
 *
 * A value of `pause` argument for so_5::send_periodic() (also for
 * so_5::environment_t::schedule_timer()) must be non-negative.
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

//! \name Common error codes.
//! \{

//! The empty name doesn't allowed.
const int rc_empty_name = 500;

/*!
 * \brief Invalid value of time limit for an agent's state.
 * \since
 * v.5.5.15
 */
const int rc_invalid_time_limit_for_state = 501;

/*!
 * \brief Feature or method has no implementation yet.
 * \since
 * v.5.5.16
 */
const int rc_not_implemented = 502;

//! Unclassified error.
const int rc_unexpected_error = 0xFFFFFF;
//! \}

} /* namespace so_5 */

