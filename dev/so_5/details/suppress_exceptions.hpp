/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.6.2
 *
 * \file
 * \brief Helpers for invoking code with suppressing exceptions.
 */

#pragma once

namespace so_5 {

namespace details {

/*!
 * @brief Helper function for execution a block of code with
 * suppression of any exceptions raised inside that block.
 *
 * All exceptions are simply intercepted. Nothing is logged in the
 * case of an exception thrown.
 *
 * @since v.5.6.2
 */
template< typename Lambda >
void suppress_exceptions( Lambda && lambda ) noexcept
{
	try
	{
		lambda();
	}
	catch( ... ) {}
}

} /* namespace details */

} /* namespace so_5 */


