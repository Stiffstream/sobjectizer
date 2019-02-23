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

namespace so_5 {

namespace disp {

namespace reuse {

//FIXME: document this!
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

//FIXME: document this!
template<
	typename Disp_Iface_Type,
	typename Disp_No_Tracking,
	typename Disp_With_Tracking,
	typename Disp_Params_Type,
	typename... Args >
std::unique_ptr< Disp_Iface_Type >
make_actual_dispatcher(
	outliving_reference_t< environment_t > env,
	const std::string & name_base,
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

