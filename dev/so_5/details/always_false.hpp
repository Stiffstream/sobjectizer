/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A simple metafunction that always "returns" false.
 *
 * \since
 * v.5.6.0
 */

#pragma once

namespace so_5 {

namespace details {

/*!
 * \brief A simple metafunction that always "returns" false.
 *
 * Can be used to trigger static_assert.
 *
 * \since
 * v.5.6.0
 */
template<typename T>
struct always_false
{
	static constexpr const bool value = false;
};

} /* namespace details */

} /* namespace so_5 */

