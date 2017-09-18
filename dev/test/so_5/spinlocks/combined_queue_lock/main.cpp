/*
 * A simple unit-test for combined_queue_lock_t.
 */

#include <so_5/disp/reuse/locks/h/locks.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace so_5::disp::reuse::locks;

void
test_thread( unsigned int milliseconds_to_wait )
{
	std::thread child;
	{
		combined_queue_lock_t lock_a;
		combined_queue_lock_t lock_b;

		combined_queue_unique_lock_t guard_b{ lock_b };

		child = std::thread{ [&] {
				combined_queue_unique_lock_t a{ lock_a };
				{
					// Informs parent that child has started.
					combined_queue_lock_guard_t b{ lock_b };
					b.notify_one();
				}

				// Try to wait of lock_a.
				a.wait_for_notify();
			} };

		guard_b.wait_for_notify();
		
		// Acquire lock_a and make child thread to wait for some time.
		combined_queue_lock_guard_t guard_a{ lock_a };
		std::this_thread::sleep_for(
				std::chrono::milliseconds{ milliseconds_to_wait } );
		guard_a.notify_one();
	}
	child.join();
}

void
do_pair_thread_test_case( unsigned int milliseconds_to_wait )
{
	std::thread parent{ test_thread, milliseconds_to_wait };
	parent.join();
}

void
single_pair_test_case()
{
	run_with_time_limit( [] {
			std::cout << "single pair: " << std::flush;
			do_pair_thread_test_case( 250 );
			std::cout << "OK" << std::endl;
		},
		2,
		"single_pair_test_case" );

}

void
serie_of_pair_tests_with_equal_intervals()
{
	std::cout << "serie of pair tests with equal interval: " << std::flush;

	for( int i = 0; i != 50; ++i )
	{
		run_with_time_limit( [] {
			do_pair_thread_test_case( 25 );
		},
		2,
		"serie_of_pair_tests_with_equal_intervals" );
	}

	std::cout << "OK" << std::endl;

}

void
serie_of_pair_tests_with_different_intervals()
{
	std::cout << "serie of pair tests with different intervals: " << std::flush;

	for( int i = 1; i != 50; ++i )
	{
		run_with_time_limit( [i] {
			do_pair_thread_test_case( i*5 );
		},
		2,
		"serie_of_pair_tests_with_different_intervals" );
	}

	std::cout << "OK" << std::endl;

}

void
bunch_of_threads()
{
	std::cout << "bunch of threads: " << std::flush;

	std::vector< std::thread > threads;
	threads.reserve( 50 );

	run_with_time_limit( [&] {
			for( std::size_t i = 0; i != 50; ++i )
				threads.push_back( std::thread{ test_thread, 250u } );

			for( auto & t : threads )
				t.join();
		},
		20,
		"bunch_of_threads" );

	std::cout << "OK" << std::endl;
}

int
main()
{
	try
	{
		single_pair_test_case();
		serie_of_pair_tests_with_equal_intervals();
		serie_of_pair_tests_with_different_intervals();
		bunch_of_threads();

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

