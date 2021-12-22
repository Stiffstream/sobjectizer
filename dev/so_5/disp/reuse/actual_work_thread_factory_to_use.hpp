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

//
// acquire_work_thread
//
/*!
 * \brief Helper function for acquiring a new worker thread from
 * an appropriate work thread factory.
 *
 * If \a params defines a factory then the factory from \a params
 * is used for acquiring new work thread. Otherwise the factory
 * from \a env is used.
 *
 * \since v.5.7.3
 */
template< typename Params >
[[nodiscard]]
work_thread_holder_t
acquire_work_thread(
	const work_thread_factory_mixin_t< Params > & params,
	environment_t & env )
	{
		auto factory = actual_work_thread_factory_to_use( params, env );
		auto & thread = factory->acquire( env );

		// This block of code shouldn't throw.
		return so_5::details::invoke_noexcept_code(
				[&]() -> work_thread_holder_t {
					return { thread, std::move(factory) };
				} );
	}

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

