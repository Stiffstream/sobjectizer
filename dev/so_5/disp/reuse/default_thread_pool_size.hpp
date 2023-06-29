/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Reusable function for the default size for a thread pool.
 */

#pragma once

#include <thread>

namespace so_5::disp::reuse
{

//
// default_thread_pool_size
//
/*!
 * \brief A helper function for detecting default thread count for
 * thread pool.
 *
 * Returns value of std::thread::hardware_concurrency() or 2 if
 * hardware_concurrency() returns 0.
 *
 * \since v.5.8.0
 */
[[nodiscard]]
inline std::size_t
default_thread_pool_size()
	{
		auto c = std::thread::hardware_concurrency();
		if( !c )
			c = 2;

		return c;
	}

} /* namespace so_5::disp::reuse */

