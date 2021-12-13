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

/*!
 * \brief Value that indicates that exceptions are enabled.
 *
 * Usage example:
 * \code
 * auto ch = so_5::create_mchain(env);
 * ...
 * so_5::close_drop_content( so_5::exceptions_enabled, ch );
 * \endcode
 *
 * \since
 * v.5.7.3
 */
inline constexpr exceptions_enabled_t exceptions_enabled{};

/*!
 * \brief Type that indicates that an exception leads to the termination of
 * the whole application.
 *
 * \since
 * v.5.7.3
 */
struct terminate_if_throws_t {};

/*!
 * \brief Value that indicates that an exception leads to the termination of
 * the whole application.
 *
 * Usage example:
 * \code
 * class some_class {
 * 	so_5::mchain_t ch_;
 * 	...
 * public:
 * 	some_class(so_5::environment_t & env)
 * 		: ch_{so_5::create_mchain(env)}
 * 	{}
 * 	~some_class() noexcept
 * 	{
 * 		// There is no sense to allow exceptions during close operation.
 * 		ch_->close(
 * 				so_5::terminate_if_throws,
 * 				so_5::mchain_props::close_mode_t::drop_content );
 * 	}
 * 	...
 * };
 * \endcode
 *
 * \since
 * v.5.7.3
 */
inline constexpr terminate_if_throws_t terminate_if_throws;

namespace details
{

/*!
 * \brief Metafunction that tells is exceptions enabled or not.
 *
 * \since
 * v.5.7.3
 */
template< typename Flag >
struct should_terminate_if_throws_t;

template<>
struct should_terminate_if_throws_t< exceptions_enabled_t >
	{
		static constexpr bool value = false;
	};

template<>
struct should_terminate_if_throws_t< terminate_if_throws_t >
	{
		static constexpr bool value = true;
	};

} /* namespace details */

} /* namespace so_5 */

