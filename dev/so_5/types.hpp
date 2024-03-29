/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Various typedefs.
*/

#pragma once

#include <atomic>
#include <cstdint>

// It is necessary that version.hpp will be almost always automatically
// included. To do that we include it in that header file because
// that file seems to be necessary for all SObjectizer's definintions.
#include <so_5/version.hpp>

namespace so_5
{

//! Atomic counter type.
using atomic_counter_t = std::atomic_ulong;


//! Atomic flag type.
using atomic_flag_t = std::atomic_ulong;

//! A type for mbox indentifier.
using mbox_id_t = unsigned long long;

/*!
 * \brief Default value for null mbox_id.
 * \since
 * v.5.5.4.1
 */
inline mbox_id_t
null_mbox_id()
	{
		return 0ull;
	}

/*!
 * \brief Thread safety indicator.
 * \since
 * v.5.4.0
 */
enum class thread_safety_t : std::uint8_t
	{
		//! Not thread safe.
		unsafe = 0,
		//! Thread safe.
		safe = 1
	};

/*!
 * \brief Shorthand for thread unsafety indicator.
 * \since
 * v.5.4.0
 */
const thread_safety_t not_thread_safe = thread_safety_t::unsafe;

/*!
 * \brief Shorthand for thread safety indicator.
 * \since
 * v.5.4.0
 */
const thread_safety_t thread_safe = thread_safety_t::safe;

/*!
 * \brief Values for dispatcher's work thread activity tracking.
 */
enum class work_thread_activity_tracking_t
	{
		//! Tracking mode is specified elsewhere.
		unspecified,
		//! Tracking is disabled.
		off,
		//! Tracking is enabled.
		on
	};

//
// message_mutability_t
//
/*!
 * \brief A enum with variants of message mutability or immutability.
 *
 * \since
 * v.5.5.19
 */
enum class message_mutability_t
	{
		immutable_message,
		mutable_message
	};

//
// message_kind_t
//
/*!
 * \brief A enum with variants of message kinds.
 *
 * \since
 * v.5.5.23
 */
enum class message_kind_t
	{
		//! Message is a signal. It means there is no data associated
		//! with the message instance.
		signal,
		//! Message is a classical message. It means that message is
		//! an instance of class derived from message_t.
		classical_message,
		//! Message is an user type message.
		user_type_message,
		//! Message is an envelope with some other message inside.
		enveloped_msg
	};

//
// coop_id_t
//
/*!
 * \brief ID of cooperation.
 *
 * \since
 * v.5.6.0
 */
using coop_id_t = std::uint_fast64_t;

//
// event_handler_kind_t
//
/*!
 * \brief Kind of an event handler.
 *
 * Since v.5.7.0 event handlers of an agent are devided into the following
 * categories:
 *
 * - final handler. That is an ordinary handler that takes a message and
 *   handles it. In the case of an enveloped message a final handler should
 *   take the payload of a message;
 * - intermediate handler. That is a special handler that doesn't handle
 *   a message by itself but delegates the handling to some other event
 *   handler or just suppresses the message. In the case of enveloped message
 *   an intermediate handler should take the whole envelope.
 *
 * \since
 * v.5.7.0
 */
enum class event_handler_kind_t : char
	{
		//! Ordinary, final event handler.
		//! That event handler should take the payload from an eveloped message.
		final_handler,
		//! Intermediate event handler. 
		//! Doesn't handle a message itself but delegates actual processing
		//! to some other event handler. Or just skips the processing of
		//! the message.
		//! That event handler should take the whole envelope.
		intermediate_handler
	};

/*!
 * \brief Possible modes of message/signal delivery.
 *
 * \since v.5.8.0
 */
enum class message_delivery_mode_t
	{
		//! Ordinary delivery. The send operation can block (for
		//! example on an attempt to send a message to a full mchain).
		ordinary,
		//! Delivery that prohibit blocking. For example a delivery
		//! of a delayed/periodic can't block the current thread
		//! (because it's the timer thread and the timer thread
		//! can't be blocked).
		//!
		//! NOTE. The current version also prohibit throwing of
		//! exceptions.
		nonblocking
	};

} /* namespace so_5 */

