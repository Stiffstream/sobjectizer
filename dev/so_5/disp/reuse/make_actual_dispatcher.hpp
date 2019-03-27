/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A set of helpers for making actual dispatcher instances.
 *
 * \since
 * v.5.6.0
 */

#pragma once

#include <so_5/stats/impl/activity_tracking.hpp>

#include <so_5/impl/internal_env_iface.hpp>

#include <string_view>

namespace so_5 {

namespace disp {

namespace reuse {

/*!
 * \brief Helper functions to adjust some dispatcher parameters with
 * respect to settings from environment.
 *
 * If dispatcher params doesn't have lock-factory specified the
 * lock-factory will be inherited from environment.
 *
 * \tparam Disp_Params_Type type of struct/class with dispatcher-related
 * parameters. This type should have queue_params() method.
 *
 * \since
 * v.5.6.0
 */
template< typename Disp_Params_Type >
void
modify_disp_params(
	so_5::environment_t & env,
	Disp_Params_Type & params )
	{
		auto lf = params.queue_params().lock_factory();
		if( !lf )
		{
			auto queue_params = params.queue_params();
			queue_params.lock_factory(
					so_5::impl::default_lock_factory( env, lf ) );
			params.set_queue_params( std::move(queue_params) );
		}
	}

/*!
 * \brief Helper function for creation of dispatcher instance with
 * respect to work thread activity tracking flag.
 *
 * \note
 * Calls modify_disp_params() before creation of dispatcher instance.
 *
 * \attention
 * Dispatcher constructor should have the following format:
 * \code
 * Dispatcher(
 * 	outliving_reference_t<environment_t> env,
 * 	const std::string_view data_source_base_name,
 * 	Disp_Params_Type disp_params,
 * 	Args && ...additional_args );
 * \endcode
 *
 * \tparam Disp_Iface_Type a type of interface for actual dispatcher. This type
 * is expected to be a base class for Disp_No_Tracking and Disp_With_Tracking
 * types.
 *
 * \tparam Disp_No_Tracking a type of dispatcher for case when thread
 * activity tracking is not used.
 *
 * \tparam Disp_With_Tracking a type of dispatcher for case when thread
 * activity tracking is used.
 *
 * \tparam Disp_Params_Type a type of dispatcher-specific parameters.
 *
 * \tparam Args types of additional arguments for dispatcher's constructor.
 *
 * \since
 * v.5.6.0
 */
template<
	typename Disp_Iface_Type,
	typename Disp_No_Tracking,
	typename Disp_With_Tracking,
	typename Disp_Params_Type,
	typename... Args >
std::unique_ptr< Disp_Iface_Type >
make_actual_dispatcher(
	outliving_reference_t< environment_t > env,
	const std::string_view name_base,
	Disp_Params_Type disp_params,
	Args && ...args )
	{
		modify_disp_params( env.get(), disp_params );

		using so_5::stats::activity_tracking_stuff::create_appropriate_disp;
		return create_appropriate_disp<
				Disp_Iface_Type,
				Disp_No_Tracking,
				Disp_With_Tracking >(
			env,
			name_base,
			std::move(disp_params),
			std::forward<Args>(args)... );
	}

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

