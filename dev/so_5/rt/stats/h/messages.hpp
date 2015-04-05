/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since v.5.5.4
 * \brief Messages with monitoring information.
 */

#pragma once

#include <so_5/rt/h/message.hpp>

#include <so_5/rt/stats/h/prefix.hpp>

namespace so_5
{

namespace rt
{

namespace stats
{

namespace messages
{

/*!
 * \since v.5.5.4
 * \brief A message with value of some quantity.
 *
 * This message can be used for monitoring things like queue sizes,
 * count of delayed messages, count of cooperations and so on.
 *
 * \tparam T type for representing quantity.
 */
template< typename T >
struct quantity : public message_t
	{
		//! Prefix of data_source name.
		prefix_t m_prefix;
		//! Suffix of data_source name.
		suffix_t m_suffix;

		//! Actual quantity value.
		T m_value;

		//! Initializing constructor.
		quantity(
			const prefix_t & prefix,
			const suffix_t & suffix,
			T value )
			:	m_prefix( prefix )
			,	m_suffix( suffix )
			,	m_value( std::move( value ) )
			{}
	};

} /* namespace messages */

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */

