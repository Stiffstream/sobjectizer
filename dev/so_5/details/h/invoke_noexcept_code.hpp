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

#include <so_5/h/compiler_features.hpp>

#if !defined(SO_5_HAVE_NOEXCEPT)
	#include <exception>
#endif

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
auto
invoke_noexcept_code( L lambda ) SO_5_NOEXCEPT -> decltype(lambda())
	{
#if defined(SO_5_HAVE_NOEXCEPT)
		// We can just rely on C++ compiler features.
		return lambda();
#else
		// C++ compiler doesn't support noexcept.
		// So we must intercept all exceptions by ourself.
		try
			{
				return lambda();
			}
		catch( ... )
			{
				std::terminate();
			}
#endif
	}

} /* namespace details */

} /* namespace so_5 */

