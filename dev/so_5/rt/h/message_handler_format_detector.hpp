/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helpers for detection of message type by message handler format.
 *
 * \since
 * v.5.5.19.5
 */

#pragma once

#include <so_5/rt/h/message.hpp>
#include <so_5/rt/h/event_data.hpp>

namespace so_5 {

namespace details {

//
// message_handler_format_detector
//
/*!
 * \brief A helper template to detect type of message from the
 * format of an event handler.
 *
 * Usage example:
 * \code
 * template<class RET, class AGENT, class HANDLER_ARG>
 * void some_method( RET (AGENT::*)(HANDLER_ARG) ) {
 * 	using message_type = typename message_handler_format_detector<
 * 			HANDLER_ARG>::type;
 * 	...
 * }
 * \endcode
 *
 * \since
 * v.5.5.19.5
 */
template<typename MESSAGE>
struct message_handler_format_detector
	{
		using type = typename message_payload_type<MESSAGE>::subscription_type;
	};

template<typename MESSAGE>
struct message_handler_format_detector<const MESSAGE &>
	{
		using type = typename message_payload_type<MESSAGE>::subscription_type;
	};

template<typename MESSAGE>
struct message_handler_format_detector<const mhood_t<MESSAGE> &>
	{
		using type = typename message_payload_type<MESSAGE>::subscription_type;
	};

template<typename MESSAGE>
struct message_handler_format_detector<mhood_t<MESSAGE>>
	{
		using type = typename message_payload_type<MESSAGE>::subscription_type;
	};

//
// is_agent_method_pointer
//
/*!
 * \brief Check whether T is a non-static member function pointer. 
 *
 * If T is not a pointer to member function then is_agent_method_pointer::value
 * will be \a false.
 *
 * If T is a pointer to method without args then:
 * * is_agent_method_pointer::value is \a true;
 * * there is a is_agent_method_pointer::agent_type type name;
 * * there is a is_agent_method_pointer::result_type type name.
 *
 * If T is a pointer to method with one argument then:
 * * is_agent_method_pointer::value is \a true;
 * * there is a is_agent_method_pointer::agent_type type name;
 * * there is a is_agent_method_pointer::result_type type name.
 * * there is a is_agent_method_pointer::argument_type type name.
 *
 * \since
 * v.5.5.20
 */
template<typename T>
struct is_agent_method_pointer
	{
		static const bool value = false;
	};

namespace agent_method_pointer_details
{
	template<typename Result, typename Agent>
	struct no_arg
		{
			using result_type = Result;
			using agent_type = Agent;
			static const bool value = true;
		};

	template<typename Result, typename Agent, typename Handler_Argument>
	struct with_arg
		{
			using result_type = Result;
			using agent_type = Agent;
			using argument_type = Handler_Argument;
			static const bool value = true;
		};

} /* namespace agent_method_pointer_details */

template<typename Result, typename Agent>
struct is_agent_method_pointer<Result (Agent::*)()>
	: public agent_method_pointer_details::no_arg<Result, Agent>
	{};

template<typename Result, typename Agent>
struct is_agent_method_pointer<Result (Agent::*)() const>
	: public agent_method_pointer_details::no_arg<Result, Agent>
	{};

template<typename Result, typename Agent, typename Handler_Argument>
struct is_agent_method_pointer<Result (Agent::*)(Handler_Argument)>
	: public agent_method_pointer_details::with_arg<Result, Agent, Handler_Argument>
	{};

template<typename Result, typename Agent, typename Handler_Argument>
struct is_agent_method_pointer<Result (Agent::*)(Handler_Argument) const>
	: public agent_method_pointer_details::with_arg<Result, Agent, Handler_Argument>
	{};

} /* namespace details */

} /* namespace so_5 */

