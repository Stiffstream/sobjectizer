/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.8
 *
 * \file
 * \brief A storage of quotes for priorities.
 */

#pragma once

#include <so_5/h/priority.hpp>
#include <so_5/h/exception.hpp>
#include <so_5/h/ret_code.hpp>

#include <algorithm>
#include <iterator>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace quoted_round_robin {

/*!
 * \since 5.5.8
 * \brief A storage of quotes for priorities.
 *
 * A usage example:
	\code
	using namespace so_5::disp::prio_one_thread::quoted_round_robin;
	quotes_t quotes{ 150 }; // Default value for all priorities.
	quotes.set( so_5::prio::p7, 350 ); // New quote for p7.
	quotes.set( so_5::prio::p6, 250 ); // New quote for p6.
		// All other quotes will be 150.
	...
	create_private_disp( env, quotes );
	\endcode

	Another example:
	\code
	using namespace so_5::disp::prio_one_thread::quoted_round_robin;
	create_private_disp( env,
		quotes_t{ 150 } // Default value for all priorites.
			.set( so_5::prio::p7, 350 ) // New quote for p7.
			.set( so_5::prio::p6, 250 ) // New quote for p6
	);
	\endcode

 * \attention Value of 0 is illegal. An exception will be throw on
 * attempt of setting 0 as a quote value.
 */
class quotes_t
	{
	public :
		//! Initializing constructor sets the default value for
		//! every priority.
		quotes_t( std::size_t default_value )
			{
				ensure_quote_not_zero( default_value );
				std::fill( std::begin(m_quotes), std::end(m_quotes), default_value );
			}

		//! Set a new quote for a priority.
		quotes_t &
		set(
			//! Priority to which a new quote to be set.
			priority_t prio,
			//! Quote value.
			std::size_t quote )
			{
				ensure_quote_not_zero( quote );
				m_quotes[ to_size_t( prio ) ] = quote;
				return *this;
			}

		//! Get the quote for a priority.
		size_t
		query( priority_t prio ) const
			{
				return m_quotes[ to_size_t( prio ) ];
			}

	private :
		//! Quotes for every priority.
		std::size_t m_quotes[ so_5::prio::total_priorities_count ];

		static void
		ensure_quote_not_zero( std::size_t value )
			{
				if( !value )
					SO_5_THROW_EXCEPTION( rc_priority_quote_illegal_value,
							"quote for a priority cannot be zero" );
			}
	};

} /* namespace quoted_round_robin */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */

