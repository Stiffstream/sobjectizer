/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.13
 *
 * \file
 * \brief Definition of various types of waiting indicators.
 */

#pragma once

namespace so_5 {

/*!
 * \since
 * v.5.5.13
 *
 * \brief A type for special marker for infitite waiting on service request
 * or on receive from %mchain.
 */
enum class infinite_wait_indication { infinite_wait };

/*!
 * \since
 * v.5.5.13
 *
 * \brief A special indicator for infinite waiting on service request or
 * on receive from %mchain.
 */
const infinite_wait_indication infinite_wait =
		infinite_wait_indication::infinite_wait;

/*!
 * \since
 * v.5.5.13
 *
 * \brief A type for special marker for no waiting on service request
 * or on receive from %mchain.
 */
enum class no_wait_indication { no_wait };

/*!
 * \since
 * v.5.5.13
 *
 * \brief A special indicator for no waiting on service request or
 * on receive from %mchain.
 */
const no_wait_indication no_wait = no_wait_indication::no_wait;

} /* namespace so_5 */

