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
#include <so_5/h/version.hpp>

namespace so_5
{

//! Atomic counter type.
typedef std::atomic_ulong atomic_counter_t;


//! Atomic flag type.
typedef std::atomic_ulong atomic_flag_t;

//! A type for mbox indentifier.
typedef unsigned long long mbox_id_t;

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
		//! Message is a service request.
		service_request,
		//! Message is an envelope with some other message inside.
		enveloped_msg
	};

} /* namespace so_5 */

