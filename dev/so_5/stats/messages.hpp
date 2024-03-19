/*
 * SObjectizer-5
 */

/*!
 * \file
 *
 * \brief Messages with monitoring information.
 *
 * \since v.5.5.4
 */

#pragma once

#include <so_5/current_thread_id.hpp>

#include <so_5/message.hpp>

#include <so_5/detect_os.hpp>

#include <so_5/stats/prefix.hpp>
#include <so_5/stats/work_thread_activity.hpp>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

namespace stats
{

namespace messages
{

/*
 * Implementation note: this type has to be marked as SO_5_TYPE for
 * clang on FreeBSD/macOS. But SO_5_TYPE shouldn't be used on Windows.
 */
#if defined(SO_5_OS_WINDOWS)
	#define SO_5_EXPORT_IMPORT_FOR_QUANTITY_MSG /* empty */
#else
	#define SO_5_EXPORT_IMPORT_FOR_QUANTITY_MSG SO_5_TYPE
#endif

/*!
 * \brief A message with value of some quantity.
 *
 * This message can be used for monitoring things like queue sizes,
 * count of delayed messages, count of cooperations and so on.
 *
 * \tparam T type for representing quantity.
 *
 * \since v.5.5.4
 */
template< typename T >
struct SO_5_EXPORT_IMPORT_FOR_QUANTITY_MSG quantity : public message_t
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

#undef SO_5_EXPORT_IMPORT_FOR_QUANTITY_MSG

/*!
 * \brief Notification about start of new stats distribution.
 *
 * \note This message is empty now but it is not declared as a signal.
 * It is for a possibility of expansion of the message in the future.
 *
 * \since v.5.5.18
 */
struct SO_5_TYPE distribution_started : public message_t {};

/*!
 * \brief Notification about finish of stats distribution.
 *
 * \note This message is empty now but it is not declared as a signal.
 * It is for a possibility of expansion of the message in the future.
 *
 * \since v.5.5.18
 */
struct SO_5_TYPE distribution_finished : public message_t {};

/*!
 * \brief Information about one work thread activity.
 *
 * \since v.5.5.18
 */
struct SO_5_TYPE work_thread_activity : public message_t
	{
		//! Prefix of data_source name.
		prefix_t m_prefix;
		//! Suffix of data_source name.
		suffix_t m_suffix;

		//! ID of the thread.
		so_5::current_thread_id_t m_thread_id;

		//! Actual value.
		work_thread_activity_stats_t m_stats;

		work_thread_activity(
			const prefix_t & prefix,
			const suffix_t & suffix,
			const so_5::current_thread_id_t & thread_id,
			work_thread_activity_stats_t stats )
			:	m_prefix( prefix )
			,	m_suffix( suffix )
			,	m_thread_id( thread_id )
			,	m_stats( stats )
			{}
	};

} /* namespace messages */

} /* namespace stats */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

