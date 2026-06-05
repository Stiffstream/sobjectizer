/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.6
 *
 * \file
 * \brief Helpers for invoking code which must not throws.
 */

#pragma once

#include <utility>

namespace so_5 {

namespace details {

/*!
 * \since
 * v.5.5.6
 *
 * \file
 * \brief Helpers for invoking code which must not throws.
 *
 * \tparam L type of lambda with main code to be invoked.
 */
template< typename L >
decltype(auto)
invoke_noexcept_code( L lambda ) noexcept
	{
		return std::forward<L>(lambda)();
	}

} /* namespace details */

} /* namespace so_5 */

