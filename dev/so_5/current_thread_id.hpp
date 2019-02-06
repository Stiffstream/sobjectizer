/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief A workaround for very slow implementation of
 * std::this_thread::get_id() under some compilers.
 *
 * \note
 * This header file was actual when too old versions of VC++ compilers
 * were used. Now it is here for compatibility reasons.
 */

#pragma once

// For the normal implementations use the standard tools.
#include <thread>

namespace so_5
{
	//! Type of the current thread id.
	typedef std::thread::id current_thread_id_t;

	//! Get the ID of the current thread.
	inline current_thread_id_t
	query_current_thread_id()
		{
			return std::this_thread::get_id();
		}

	//! Get NULL thread id.
	inline current_thread_id_t
	null_current_thread_id()
		{
			return std::thread::id();
		}

	/*!
	 * \brief Get the raw thread id from current_thread_id.
	 *
	 * \since
	 * v.5.5.18
	 */
	inline std::thread::id
	raw_id_from_current_thread_id( const current_thread_id_t & w )
		{
			return w;
		}

} /* namespace so_5 */

