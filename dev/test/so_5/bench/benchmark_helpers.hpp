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
				auto finish_time = std::chrono::high_resolution_clock::now();
				const double duration =
						std::chrono::duration_cast< std::chrono::milliseconds >(
								finish_time - m_start ).count() / 1000.0;
				const double price = duration / events;
				const double throughtput = 1 / price;

				std::cout.precision( 10 );
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

				std::cout.precision( 10 );
				std::cout << m_name << ": "
					<< std::chrono::duration_cast< std::chrono::milliseconds >(
							finish - m_start ).count() / 1000.0 << "s"
					<< std::endl;
			}

	private :
		const std::string m_name;
		const std::chrono::high_resolution_clock::time_point m_start;
	};

