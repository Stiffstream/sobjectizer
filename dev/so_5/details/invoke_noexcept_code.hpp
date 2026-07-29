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

#if !defined( SO_5_DETAILS_INVOKE_NOEXCEPT_CODE_HPP )
#define SO_5_DETAILS_INVOKE_NOEXCEPT_CODE_HPP

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

#endif

