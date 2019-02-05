/*
 * SObjectizer-5
 */
/*!
 * \since v.5.4.0
 * \file
 * \brief A simple helpers for building benchmarks.
 */
#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>

namespace benchmarks_details {

//! Helper class for changing and restoring ostream precision settings.
class precision_settings_t
	{
		std::ios_base & m_what;
		std::streamsize m_old_value;

	public :
		precision_settings_t(
			std::ios_base & what,
			std::streamsize new_value )
			:	m_what( what )
			,	m_old_value( what.precision( new_value ) )
			{}
		~precision_settings_t()
			{
				m_what.precision( m_old_value );
			}
	};

} /* namespace benchmarks_details */

//! A helper for fixing starting and finishing time points and
//! calculate events processing time and events throughtput.
class benchmarker_t
	{
	public :
		//! Fix starting time.
		inline void
		start()
			{
				m_start = std::chrono::high_resolution_clock::now();
			}

		//! Fix finish time and show stats.
		inline void
		finish_and_show_stats(
			unsigned long long events,
			const std::string & title )
			{
				if( !events )
					throw std::invalid_argument( "events cannot be 0" );

				auto finish_time = std::chrono::high_resolution_clock::now();
				const double duration =
						std::chrono::duration_cast< std::chrono::milliseconds >(
								finish_time - m_start ).count() / 1000.0;
				const double price = duration / events;
				const double throughtput = 1 / price;

				benchmarks_details::precision_settings_t precision{ std::cout, 10 };
				std::cout << title << ": " << events
						<< ", total_time: " << duration << "s"
						<< "\n""price: " << price << "s"
						<< "\n""throughtput: " << throughtput << " " << title << "/s"
						<< std::endl;
			}

	private :
		std::chrono::high_resolution_clock::time_point m_start;
	};

//! A helper for showing duration between constructor and destructor calls.
/*!
 * Usage example:
\code
{
	duration_meter_t meter( "creating some objects" );
	... // Some code here
} // Duration of the code above will be shown here.
\endcode
*/
class duration_meter_t
	{
	public :
		duration_meter_t( std::string name )
			:	m_name( std::move( name ) )
			,	m_start( std::chrono::high_resolution_clock::now() )
			{}

		~duration_meter_t()
			{
				auto finish = std::chrono::high_resolution_clock::now();

				benchmarks_details::precision_settings_t precision{ std::cout, 10 };
				std::cout << m_name << ": "
					<< std::chrono::duration_cast< std::chrono::milliseconds >(
							finish - m_start ).count() / 1000.0 << "s"
					<< std::endl;
			}

	private :
		const std::string m_name;
		const std::chrono::high_resolution_clock::time_point m_start;
	};

