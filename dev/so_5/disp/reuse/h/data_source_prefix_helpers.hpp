/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \file
 * \brief Helpers creation of data source name prefixes.
 */

#pragma once

#include <so_5/rt/stats/h/prefix.hpp>

#include <sstream>

namespace so_5 {

namespace disp {

namespace reuse {

namespace ios_helpers {

/*!
 * \since v.5.5.4
 * \brief Helper for showing only part of long string.
 */
struct length_limited_string
	{
		const std::string & m_what;
		const std::size_t m_limit;

		length_limited_string(
			const std::string & what,
			std::size_t limit )
			:	m_what( what )
			,	m_limit( limit )
			{}
	};

inline std::ostream &
operator<<( std::ostream & to, const length_limited_string & v )
	{
		if( v.m_what.size() > v.m_limit )
			{
				const std::size_t median = v.m_limit / 2;

				to << v.m_what.substr( 0, median )
					<< "..."
					<< v.m_what.substr( v.m_what.size() - median + 3 );
			}
		else
			to << v.m_what;

		return to;
	}

/*!
 * \since v.5.5.4
 * \brief Helper for showing pointer value.
 */
struct pointer
	{
		const void * m_what;

		pointer( const void * what ) : m_what{ what } {}
	};

inline std::ostream &
operator<<( std::ostream & to, const pointer & v )
	{
		auto oldf = to.setf( std::ios_base::hex, std::ios_base::basefield );
		to << "0x" << std::ptrdiff_t{
				reinterpret_cast< const char * >(v.m_what) -
				static_cast< const char * >(nullptr)};
		to.setf( oldf, std::ios_base::basefield );

		return to;
	}

} /* namespace ios_helpers */

/*!
 * \since v.5.5.4
 * \brief Create basic prefix for dispatcher data source names.
 */
inline so_5::rt::stats::prefix_t
make_disp_prefix(
	//! Textual marker of dispatcher type (like ot, ao, ag, tp, atp, ...).
	const char * disp_type,
	//! User-supplied optional base for name. Can be empty.
	const std::string data_sources_name_base,
	//! Pointer to dispatcher instance.
	//! Will be used if \a data_sources_name_base is empty.
	const void * disp_this_pointer )
	{
		const std::size_t max_name_base_fragment = 24;
		std::ostringstream ss;

		ss << "disp/" << disp_type << "/";
		if( !data_sources_name_base.empty() )
			ss << ios_helpers::length_limited_string{
					data_sources_name_base,
					max_name_base_fragment };
		else
			ss << ios_helpers::pointer{ disp_this_pointer };

		return so_5::rt::stats::prefix_t{ ss.str() };
	}

/*!
 * \since v.5.5.4
 * \brief Create prefix for dispatcher's working thread data source.
 */
inline so_5::rt::stats::prefix_t
make_disp_working_thread_prefix(
	//! Prefix for the dispatcher.
	const so_5::rt::stats::prefix_t & disp_prefix,
	//! Thread ordinal number.
	std::size_t thread_number )
	{
		std::ostringstream ss;
		ss << disp_prefix.c_str() << "/wt-" << thread_number;

		return ss.str();
	}

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

