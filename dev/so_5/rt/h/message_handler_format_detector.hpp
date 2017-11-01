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

} /* namespace details */

} /* namespace so_5 */

