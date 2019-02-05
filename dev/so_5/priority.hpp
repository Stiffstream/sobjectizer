/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.8
 *
 * \brief Definition of priority type.
 */

#pragma once

#include <type_traits>
#include <stdexcept>
#include <string>

namespace so_5 {

/*!
 * \since
 * v.5.5.8
 *
 * \brief Definition of supported priorities.
 */
enum class priority_t : unsigned char
	{
		p_min = 0,
		p0 = p_min,
		p1,
		p2,
		p3,
		p4,
		p5,
		p6,
		p7,
		p_max = p7
	};

/*!
 * \since
 * v.5.5.8
 *
 * \brief Helper function for conversion from priority to size_t.
 */
inline std::size_t
to_size_t( priority_t priority )
	{
		return static_cast< std::size_t >( priority );
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief Helper function for conversion from size_t to priority.
 *
 * \throw std::invalid_argument if \a v is out of range.
 */
inline priority_t
to_priority_t( std::size_t v )
	{
		if( !( v >= to_size_t(priority_t::p_min) &&
				v <= to_size_t(priority_t::p_max) ) )
			throw std::invalid_argument( "value to be converted to "
					"priority_t is out of range: " +
					std::to_string( v ) );

		return static_cast< priority_t >( v );
	}

namespace prio {

/*!
 * \name Constants with priority values.
 * \{
 */
const priority_t p0 = priority_t::p0;
const priority_t p1 = priority_t::p1;
const priority_t p2 = priority_t::p2;
const priority_t p3 = priority_t::p3;
const priority_t p4 = priority_t::p4;
const priority_t p5 = priority_t::p5;
const priority_t p6 = priority_t::p6;
const priority_t p7 = priority_t::p7;
/*!
 * \}
 */

/*!
 * \since
 * v.5.5.8
 *
 * \brief Default priority value.
 */
const priority_t default_priority = p0;

/*!
 * \since
 * v.5.5.8
 *
 * \brief Total count of priorities.
 */
const unsigned int total_priorities_count =
	static_cast< unsigned int >( p7 ) -
	static_cast< unsigned int >( p0 ) + 1;

/*!
 * \since
 * v.5.5.8
 *
 * \brief Is there higher priority?
 */
inline bool
has_next( priority_t p ) { return p != priority_t::p_max; }

/*!
 * \since
 * v.5.5.8
 *
 * \brief Get the next priority value.
 *
 * \return next priority value if \a p is less than priority_t::p_max or
 * priority_t::p_max instead.
 */
inline priority_t
next( priority_t p )
	{
		using ut = std::underlying_type< priority_t >::type;
		return p < priority_t::p_max ?
				(static_cast< priority_t >( static_cast< ut >(p) + 1))
				: priority_t::p_max;
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief Is there lower priority?
 */
inline bool
has_prev( priority_t p ) { return p != priority_t::p_min; }

/*!
 * \since
 * v.5.5.8
 *
 * \brief Get the previous priority value.
 *
 * \return previous priority value if \a p is greater than priority_t::p_min or
 * priority_t::p_min instead.
 */
inline priority_t
prev( priority_t p )
	{
		using ut = std::underlying_type< priority_t >::type;
		return p > priority_t::p_min ?
				(static_cast< priority_t >(static_cast< ut >(p) - 1 ))
				: priority_t::p_min;
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief Does enumeration of all priorities.
 *
 * \note Performs enumeration from priority_t::p_min to priority_t::p_max
 * (inclusive).
 *
 * \tparam Lambda a lambda with the prototype:
 * \code
	return_type lambda(so_5::priority_t);
 * \endcode
 * Return value from that lambda will be ignored and discarded.
 *
 * \par Example:
	\code
	// Fill the new cooperation with agents with different priorities.
	env.introduce_coop(
		so_5::disp::prio_one_thread::strictly_ordered::create_private_disp(env)->binder(),
		[&]( so_5::coop_t & coop ) {
			so_5::prio::for_each_priority( [&]( so_5::priority_t prio ) {
					// Creation of new agent with next priority.
					coop.make_agent< my_prio_agent >( prio, ... );
				} );
		} );
	\endcode
 */
template< typename Lambda >
void
for_each_priority( Lambda l )
	{
		using ut = std::underlying_type< priority_t >::type;
		const auto min = static_cast< ut >( priority_t::p_min );
		const auto max = static_cast< ut >( priority_t::p_max );
		for( auto i = min; i <= max; ++i )
			l( static_cast< priority_t >( i ) );
	}

} /* namespace prio */

} /* namespace so_5 */

