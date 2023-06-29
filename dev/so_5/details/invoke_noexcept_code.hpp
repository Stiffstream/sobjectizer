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
auto
invoke_noexcept_code( L lambda ) noexcept -> decltype(lambda())
	{
		return lambda();
	}

} /* namespace details */

} /* namespace so_5 */

