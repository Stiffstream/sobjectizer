/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A helper function for the creation of a new mchain.
 *
 * \since
 * v.5.7.0
 */

#pragma once

#include <so_5/impl/mchain_details.hpp>
#include <so_5/impl/msg_tracing_helpers.hpp>

namespace so_5
{

namespace impl
{

/*!
 * \brief Helper function for creation of a new mchain with respect
 * to message tracing.
 *
 * This function was in mbox_core.cpp file until v.5.7.0 and is
 * placed in a separate header file in v.5.7.0.
 *
 * \tparam Q type of demand queue to be used for new mchain.
 * \tparam A type of arguments for mchain_template constructor.
 */
template< typename Q, typename... A >
[[nodiscard]] mchain_t
make_mchain(
	outliving_reference_t< so_5::msg_tracing::holder_t > tracer,
	const mchain_params_t & params,
	A &&... args )
	{
		using namespace so_5::mchain_props;
		using namespace so_5::impl::msg_tracing_helpers;
		using D = mchain_tracing_disabled_base;
		using E = mchain_tracing_enabled_base;

		if( tracer.get().is_msg_tracing_enabled()
				&& !params.msg_tracing_disabled() )
			return mchain_t{
					new mchain_template< Q, E >{
						std::forward<A>(args)...,
						params,
						tracer } };
		else
			return mchain_t{
					new mchain_template< Q, D >{
						std::forward<A>(args)..., params } };
	}

} /* namespace impl */

} /* namespace so_5 */

