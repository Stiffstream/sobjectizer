/*
 * A test for so_5::details::remaining_time_counter.
 *
 */

#include <so_5/details/h/remaining_time_counter.hpp>

#include <various_helpers_1/ensure.hpp>

#include <cstdlib>
#include <iostream>
#include <thread>

int
main()
{
	const std::chrono::seconds pause(2);

	const auto started_at = std::chrono::steady_clock::now();
	so_5::details::remaining_time_counter_t counter( pause );
	for( int i = 0; i < 5 && counter; ++i )
	{
		std::cout << "sleeping #" << i << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds(500) );
		counter.update();
	}

	const auto finished_at = std::chrono::steady_clock::now();
	ensure_or_die( pause <= (finished_at - started_at),
			"at least 2s must be passed " );
	ensure_or_die( !counter, "no more time must left" );

	return 0;
}

