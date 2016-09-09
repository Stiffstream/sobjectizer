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

} /* namespace so_5 */

