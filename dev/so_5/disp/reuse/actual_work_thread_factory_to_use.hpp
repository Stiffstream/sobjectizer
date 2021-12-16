/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helper for detection of actual work thread factory to use.
 *
 * \since v.5.7.3
 */

#pragma once

#include <so_5/disp/reuse/work_thread_factory_params.hpp>

#include <so_5/environment.hpp>

namespace so_5 {

namespace disp {

namespace reuse {

//
// actual_work_thread_factory_to_use
//
// IMPLEMENTATION NOTE: this function can't be defined in
// so_5/disp/reuse/work_thread_factory_params.hpp because
// it needs a full definition of so_5::environment_t class.
// This definition isn't available yet, because the header
// so_5/disp/reuse/work_thread_factory_params.hpp is included
// in so_5/environment.hpp.
//
/*!
 * \brief Helper to detect actual work thread factory to be used.
 *
 * If \a params defines a factory then the factory from \a params
 * is returned. Otherwise, the factory from \a env is returned.
 */
template< typename Params >
[[nodiscard]]
abstract_work_thread_factory_shptr_t
actual_work_thread_factory_to_use(
	const work_thread_factory_mixin_t< Params > & params,
	const environment_t & env ) noexcept
	{
		auto result = params.work_thread_factory();
		if( !result )
			result = env.work_thread_factory();
		return result;
	}

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

