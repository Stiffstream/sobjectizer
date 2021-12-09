/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Types and constants for exception control flags.
 */

#pragma once

namespace so_5
{

/*!
 * \brief Type that indicates that exceptions are enabled.
 *
 * \since
 * v.5.7.3
 */
struct exceptions_enabled_t {};

//FIXME: usage example has to be provided.
/*!
 * \brief Value that indicates that exceptions are enabled.
 *
 * \since
 * v.5.7.3
 */
inline constexpr exceptions_enabled_t exceptions_enabled{};

/*!
 * \brief Type that indicates that an exception leads to the termination.
 *
 * \since
 * v.5.7.3
 */
struct terminate_if_throws_t {};

//FIXME: usage example has to be provided.
/*!
 * \brief Value that indicates that an exception leads to the termination.
 *
 * \since
 * v.5.7.3
 */
inline constexpr terminate_if_throws_t terminate_if_throws;

} /* namespace so_5 */

