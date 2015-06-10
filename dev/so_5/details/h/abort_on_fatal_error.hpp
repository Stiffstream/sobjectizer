/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.6
 * \file
 * \brief Helpers for aborting the whole application in the
 * case of a fatal error.
 */

#pragma once

#include <so_5/details/h/invoke_noexcept_code.hpp>

#include <cstdlib>

namespace so_5 {

namespace details {

/*!
 * \since v.5.5.6
 * \file
 * \brief Helper for aborting the whole application in the
 * case of a fatal error.
 *
 * \tparam L type of lambda with logging action. Note that this
 * lambda must not throws exceptions.
 */
template< typename L >
void
abort_on_fatal_error( L logging_lambda ) SO_5_NOEXCEPT
	{
		invoke_noexcept_code( std::move( logging_lambda ) );
		std::abort();
	}

} /* namespace details */

} /* namespace so_5 */

